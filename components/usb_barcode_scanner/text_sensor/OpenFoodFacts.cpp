#include <algorithm>
#include <cstring>

#include "esphome/core/application.h"
#include "esphome/core/log.h"

#include "cJSON.h"
#include "OpenFoodFacts.h"

namespace esphome {
namespace usb_barcode_scanner {

    static const char *TAG = "OpenFoodFacts";

    const int HTTP_OUTPUT_BUFFER = 1024;

    static std::string sanitize_utf8(const std::string &input) {
        std::string output;
        output.reserve(input.size());
        const unsigned char *data = reinterpret_cast<const unsigned char *>(input.data());
        size_t i = 0;

        while (i < input.size()) {
            unsigned char c = data[i];
            if (c < 0x80) {
                output.push_back(static_cast<char>(c));
                i++;
                continue;
            }

            size_t expected = 0;
            if (c >= 0xC2 && c <= 0xDF) {
                expected = 2;
            } else if (c >= 0xE0 && c <= 0xEF) {
                expected = 3;
            } else if (c >= 0xF0 && c <= 0xF4) {
                expected = 4;
            } else {
                output.push_back('?');
                i++;
                continue;
            }

            if (i + expected > input.size()) {
                output.push_back('?');
                i++;
                continue;
            }

            bool valid = true;
            for (size_t j = 1; j < expected; j++) {
                if ((data[i + j] & 0xC0) != 0x80) {
                    valid = false;
                    break;
                }
            }

            if (!valid) {
                output.push_back('?');
                i++;
                continue;
            }

            output.append(reinterpret_cast<const char *>(data + i), expected);
            i += expected;
        }

        return output;
    }

    static esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
        App.feed_wdt();
        OpenFoodFacts *self = reinterpret_cast<OpenFoodFacts *>(evt->user_data);
        if (evt->event_id == HTTP_EVENT_ON_CONNECTED || evt->event_id == HTTP_EVENT_ON_FINISH) {
            if (self) {
                self->bytes_read = 0;
            }
        }
        if (evt->event_id == HTTP_EVENT_ON_DATA && self) {
            int copy_len = std::min(evt->data_len, (HTTP_OUTPUT_BUFFER - self->bytes_read));
            if (copy_len > 0) {
                memcpy(self->receive_buffer + self->bytes_read, evt->data, copy_len);
                self->bytes_read += copy_len;
            }
            if (self->bytes_read <= HTTP_OUTPUT_BUFFER) {
                self->receive_buffer[self->bytes_read] = 0;
            }
            if (copy_len < evt->data_len) {
                ESP_LOGE(TAG, "HTTP response exceeded buffer size (%d bytes total)", self->bytes_read + (evt->data_len - copy_len));
            }
        }
        if (evt->event_id == HTTP_EVENT_ERROR) {
            ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
        }
        if (evt->event_id == HTTP_EVENT_DISCONNECTED) {
            ESP_LOGD(TAG, "DISCONNECT");
            if (self) {
                self->bytes_read = 0;
            }
        }
        return ESP_OK;
    }

    OpenFoodFacts::OpenFoodFacts() {
        this->receive_buffer = new char[HTTP_OUTPUT_BUFFER + 1];
        this->bytes_read = 0;
        this->client = nullptr;
        if (receive_buffer == nullptr) {
            ESP_LOGE(TAG, "Failed to allocate memory for buffer");
            ESP_LOGE(TAG, "Available heap: %" PRIu32, esp_get_free_heap_size());
        }
    }

    OpenFoodFacts::~OpenFoodFacts() {
        delete[] this->receive_buffer;
        this->receive_buffer = nullptr;
    }

    esp_http_client_handle_t OpenFoodFacts::initHttpClient() {
        memset(this->receive_buffer, 0, HTTP_OUTPUT_BUFFER + 1);
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
        esp_http_client_config_t config = { nullptr };
        #pragma GCC diagnostic pop
        config.method = HTTP_METHOD_GET;
        config.buffer_size = HTTP_OUTPUT_BUFFER;
        config.user_data = this;
        config.event_handler = _http_event_handler;
        config.timeout_ms = 5000;
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
        ESP_LOGI(TAG, receive_buffer);

        auto root = cJSON_Parse(receive_buffer);
        if (!root) {
            ESP_LOGE(TAG, "JSON parse failed");
            return optional<std::string>();
        }
        auto result = cJSON_GetObjectItem(root, "result");
        if (!cJSON_IsObject(result)) {
            ESP_LOGE(TAG, "Missing result object");
            cJSON_Delete(root);
            return optional<std::string>();
        }
        auto id_item = cJSON_GetObjectItem(result, "id");
        if (!cJSON_IsString(id_item)) {
            ESP_LOGE(TAG, "Missing result.id");
            cJSON_Delete(root);
            return optional<std::string>();
        }
        const char *found = id_item->valuestring;
        if (strcmp("product_found", found) != 0) {
            ESP_LOGE(TAG, "Barcode not found: %s", cJSON_Print(root));
            cJSON_Delete(root);
            return optional<std::string>();
        }

        auto product = cJSON_GetObjectItem(root, "product");
        if (!cJSON_IsObject(product)) {
            ESP_LOGE(TAG, "Missing product object");
            cJSON_Delete(root);
            return optional<std::string>();
        }

        auto name_item = cJSON_GetObjectItem(product, "product_name");
        auto brand_item = cJSON_GetObjectItem(product, "brands");

        if (!cJSON_IsString(name_item) || !cJSON_IsString(brand_item)) {
            cJSON_Delete(root);
            return optional<std::string>();
        }

        const char *name_ptr = cJSON_GetObjectItem(product, "product_name")->valuestring;
        const char *brands_ptr = cJSON_GetObjectItem(product, "brands")->valuestring;
        std::string raw_name = name_ptr ? name_ptr : "";
        std::string raw_brands = brands_ptr ? brands_ptr : "";
        std::string safe_name = sanitize_utf8(raw_name);
        std::string safe_brands = sanitize_utf8(raw_brands);
        if (safe_name != raw_name || safe_brands != raw_brands) {
            ESP_LOGW(TAG, "Sanitized invalid UTF-8 in OpenFoodFacts response");
        }
        cJSON_Delete(root);

        ESP_LOGI(TAG, "%s (%s)", safe_name.c_str(), safe_brands.c_str());
        return safe_name + " (" + safe_brands + ")";
    }

    void OpenFoodFacts::set_region(std::string region) {
        this->region = region;
    }

    std::string OpenFoodFacts::get_region() {
        return this->region;
    }
}
}
