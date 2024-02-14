#include <algorithm>

#include "esphome/core/application.h"
#include "esphome/core/log.h"

#include "cJSON.h"
#include "OpenFoodFacts.h"

namespace esphome {
namespace usb_barcode_scanner {

    static const char *TAG = "OpenFoodFacts";

    const int HTTP_OUTPUT_BUFFER = 1024;

    esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
        App.feed_wdt();
        static int bytes_read = 0;
        if (evt->event_id == HTTP_EVENT_ON_CONNECTED || evt->event_id == HTTP_EVENT_ON_FINISH) {
            bytes_read = 0;
        }
        if (evt->event_id == HTTP_EVENT_ON_DATA) {
            int copy_len = std::min(evt->data_len, (HTTP_OUTPUT_BUFFER - bytes_read));
            if (copy_len) {
                memcpy(((char*)evt->user_data) + bytes_read, evt->data, copy_len);
                ((char*)evt->user_data)[bytes_read + copy_len] = 0;
                bytes_read += copy_len;
            } 
        }
        if (evt->event_id == HTTP_EVENT_ERROR) {
            ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
        }
        if (evt->event_id == HTTP_EVENT_DISCONNECTED) {
            ESP_LOGD(TAG, "DISCONNECT");
            bytes_read = 0;
        }
        return ESP_OK;
    }

    OpenFoodFacts::OpenFoodFacts() {
        this->receive_buffer = new char[HTTP_OUTPUT_BUFFER + 1];
        if (receive_buffer == nullptr) {
            ESP_LOGE(TAG, "Failed to allocate memory for buffer");
            ESP_LOGE(TAG, "Available heap: %" PRIu32, esp_get_free_heap_size());
        }
    }

    OpenFoodFacts::~OpenFoodFacts() {
        delete[] this->receive_buffer;
    }

    esp_http_client_handle_t OpenFoodFacts::initHttpClient() {
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
        esp_http_client_config_t config = { nullptr };
        #pragma GCC diagnostic pop
        config.method = HTTP_METHOD_GET;
        config.buffer_size = HTTP_OUTPUT_BUFFER;
        config.user_data = receive_buffer;
        config.event_handler = _http_event_handler;
        config.timeout_ms=5000;
        config.is_async = true;
        config.url = "https://de.openfoodfacts.org/api/v3/product/";
        return esp_http_client_init(&config);
    }

    void OpenFoodFacts::cleanHttpClient(esp_http_client_handle_t client) {
        esp_http_client_cleanup(client);
    }

    optional<std::string> OpenFoodFacts::getListItemFromBarcode(const std::string& barcode) {
        this->state.data.barcode = barcode;
        
        if (this->state.function != NONE) {
            this->state.function = NONE;
            this->state.exit = false;
            ESP_LOGD(TAG, "resume from yield");
            goto RETURN_SPOT;
        }

        this->client = initHttpClient();
        esp_http_client_set_url(this->client, ("https://" + this->region + ".openfoodfacts.org/api/v3/product/" + this->state.data.barcode + ".json?fields=product_name,brands,abbreviated_product_name").c_str());
        
        esp_err_t err;
        while(1) {
            RETURN_SPOT:
            err = esp_http_client_perform(this->client);
            if (err != ESP_ERR_HTTP_EAGAIN) {
                break;
            }
            this->state.function = GET_LIST_ITEM_FROM_BARCODE;
            this->state.exit = true;
            ESP_LOGD(TAG, "yield");
            return optional<std::string>();
        }

        if (err != ESP_OK) {
            ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
            cleanHttpClient(this->client);
            return getListItemFromBarcode(this->state.data.barcode);
        }
        int status_code = esp_http_client_get_status_code(this->client);
        if (status_code != HttpStatus_Ok) {
            ESP_LOGE(TAG, "HTTP request status code: %d", status_code);
            cleanHttpClient(this->client);
            return optional<std::string>();
        }
        
        cleanHttpClient(this->client);
        //ESP_LOGI(TAG, receive_buffer);

        auto root = cJSON_Parse(receive_buffer);
        auto result = cJSON_GetObjectItem(root, "result");
        auto found = cJSON_GetObjectItem(result, "id")->valuestring;
        if (strcmp("product_found", found) != 0) {
            ESP_LOGE(TAG, "Barcode not found: %s", cJSON_Print(root));
            return optional<std::string>();
        }

        auto product = cJSON_GetObjectItem(root, "product");
        auto name = cJSON_GetObjectItem(product, "product_name")->valuestring;
        auto brands = cJSON_GetObjectItem(product, "brands")->valuestring;
        return std::string(name) + " (" + std::string(brands) + ")";
    }

    void OpenFoodFacts::set_region(std::string region) {
        this->region = region;
    }

    std::string OpenFoodFacts::get_region() {
        return this->region;
    }
}
}
