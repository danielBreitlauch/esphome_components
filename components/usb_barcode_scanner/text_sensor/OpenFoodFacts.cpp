#include <algorithm>

#include "esphome/core/application.h"
#include "esphome/core/log.h"

#include "esp_tls.h"
#include "esp_crt_bundle.h"

#include "cJSON.h"
#include "OpenFoodFacts.h"

namespace esphome {
namespace usb_barcode_scanner{

    static const char *TAG = "OpenFoodFacts";

    const int HTTP_OUTPUT_BUFFER = 4096;

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
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGE(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGE(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            bytes_read = 0;
        }
        return ESP_OK;
    }

    OpenFoodFacts::OpenFoodFacts() {
        this->receive_buffer = new char[HTTP_OUTPUT_BUFFER + 1];
        if (receive_buffer == nullptr) {
            ESP_LOGE(TAG, "Failed to allocate memory for buffer");
            ESP_LOGE(TAG, "Available heap: %" PRIu32, esp_get_free_heap_size());
            exit(-1); // TODO: remove
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
        config.crt_bundle_attach = esp_crt_bundle_attach;
        config.buffer_size = HTTP_OUTPUT_BUFFER;
        config.user_data = receive_buffer;
        config.event_handler = _http_event_handler;
        config.keep_alive_enable = true;
        config.timeout_ms=20000;
        config.url = "https://de.openfoodfacts.org/api/v3/product/";
        return esp_http_client_init(&config);
    }

    void OpenFoodFacts::cleanHttpClient(esp_http_client_handle_t client) {
        esp_http_client_cleanup(client);
    }

    optional<std::string> OpenFoodFacts::getListItemFromBarcode(std::string barcode) {
        auto client = initHttpClient();
        esp_http_client_set_url(client, ("https://" + this->region + ".openfoodfacts.org/api/v3/product/" + barcode + ".json?fields=product_name,brands,abbreviated_product_name").c_str());

        esp_err_t err = esp_http_client_perform(client);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
            cleanHttpClient(client);
            return getListItemFromBarcode(barcode);
        }
        int status_code = esp_http_client_get_status_code(client);
        if (status_code != HttpStatus_Ok) {
            ESP_LOGE(TAG, "HTTP request status code: %d", status_code);
            cleanHttpClient(client);
            return optional<std::string>();
        }
        
        cleanHttpClient(client);
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
