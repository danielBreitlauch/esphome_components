#include <algorithm>
#include <random>
#include<tuple>

#include "esphome/core/application.h"
#include "esphome/core/log.h"

#include "cJSON.h"
#include "miniz.h"

#include "Paprika3List.h"

namespace esphome {
namespace paprika_app_list {

    static const char *TAG = "Paprika3List";

    const int HTTP_OUTPUT_BUFFER = 131072;

    esp_err_t _http_event_handler2(esp_http_client_event_t *evt) {
        App.feed_wdt();
        static int bytes_read = 0;
        if (evt->event_id == HTTP_EVENT_ON_CONNECTED || evt->event_id == HTTP_EVENT_ON_FINISH) {
            bytes_read = 0;
        }
        if (evt->event_id == HTTP_EVENT_ON_DATA) {
            int copy_len = std::min(evt->data_len, (HTTP_OUTPUT_BUFFER - bytes_read));
            if (copy_len) {
                memcpy(((char*)evt->user_data) + bytes_read, evt->data, copy_len);
                ((char*)evt->user_data)[bytes_read + copy_len + 1] = 0;
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

    Paprika3List::Paprika3List() {
        this->receive_buffer = new char[HTTP_OUTPUT_BUFFER + 1];
        if (receive_buffer == nullptr) {
            ESP_LOGE(TAG, "Failed to allocate memory for buffer");
            ESP_LOGE(TAG, "Available heap: %" PRIu32, esp_get_free_heap_size());
            exit(-1);
        }
    }

    Paprika3List::~Paprika3List() {
        delete[] this->receive_buffer;
    }

    esp_http_client_handle_t Paprika3List::initHttpClient() {
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
        esp_http_client_config_t config = { nullptr };
        #pragma GCC diagnostic pop
        config.method = HTTP_METHOD_POST;
        config.buffer_size = HTTP_OUTPUT_BUFFER;
        config.user_data = receive_buffer;
        config.event_handler = _http_event_handler2;
        config.timeout_ms = 10000;
        config.url = "https://de.openfoodfacts.org/api/v3/product/";
        return esp_http_client_init(&config);
    }

    void Paprika3List::cleanHttpClient(esp_http_client_handle_t client) {
        esp_http_client_cleanup(client);
    }

    void Paprika3List::updateListItem(const std::string& name) {
        if (this->paprikaBearerToken.length() == 0) {
            login();
        }

        auto perhapsItems = this->list();
        if (!perhapsItems.has_value()) {
            return;
        }
        auto items = *perhapsItems;
        auto newItem = ListItem(name);
        for (auto &listItem : items) {
            if (newItem == listItem) {
                ESP_LOGI(TAG, "found existing item: %s", listItem.name.c_str());
                return;
            }
        }
        this->createItem(newItem);
    }

    void Paprika3List::login() {
        const std::string BOUNDARY = "__X_PAW_BOUNDARY__";
        const std::string post_data = "--" + BOUNDARY + "\r\n" + 
                                "Content-Disposition: form-data; name=\"email\"\r\n" +
                                "\r\n" + this->email + 
                                "\r\n--" + BOUNDARY + "\r\n" +
                                "Content-Disposition: form-data; name=\"password\"\r\n" + 
                                "\r\n" + this->password +
                                "\r\n--" + BOUNDARY + "--\r\n";

        auto client = initHttpClient();
        esp_http_client_set_method(client, HTTP_METHOD_POST);
        esp_http_client_set_url(client, (this->paprikaRestURLv1 + "account/login/").c_str());
        esp_http_client_set_post_field(client, post_data.c_str(), strlen(post_data.c_str()));
        esp_http_client_set_header(client, "Content-Type", ("multipart/form-data; charset=utf-8; boundary=" + BOUNDARY).c_str());

        esp_err_t err = esp_http_client_perform(client);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
            cleanHttpClient(client);
            login();
            return;
        }
        int status_code = esp_http_client_get_status_code(client);
        if (status_code != HttpStatus_Ok) {
            ESP_LOGE(TAG, "Login failed. HTTP request status code: %d", status_code);
            this->status_set_warning();
            this->paprikaBearerToken = "";
            return;
        }
        cleanHttpClient(client);

        auto root = cJSON_Parse(receive_buffer);
        auto result = cJSON_GetObjectItem(root, "result");
        this->paprikaBearerToken = cJSON_GetObjectItem(result, "token")->valuestring;
        ESP_LOGI(TAG, "token: %s", this->paprikaBearerToken.c_str());
        this->status_clear_warning();
    }

    optional<std::vector<ListItem>> extractItems(const char* buffer, const std::string& listID) {
        auto root = cJSON_Parse(buffer);
        auto results = cJSON_GetObjectItem(root, "result");
        if (!cJSON_IsArray(results)) {
            ESP_LOGE(TAG, "Response is no json: %s", buffer);
            ESP_LOGE(TAG, "Results is not an array: %s", cJSON_Print(results));
		    cJSON_Delete(root);
            return optional<std::vector<ListItem>>();
	    }
        
        auto list = std::vector<ListItem>();
        cJSON *iterator = NULL;
        cJSON_ArrayForEach(iterator, results) {
            std::string listUID = cJSON_GetObjectItem(iterator, "list_uid")->valuestring;
            if (listUID == listID) {
                std::string uid = cJSON_GetObjectItem(iterator, "uid")->valuestring;
                std::string name = cJSON_GetObjectItem(iterator, "name")->valuestring;
			    ESP_LOGD(TAG, "uid: %s, name: %s", uid.c_str(), name.c_str());
                list.emplace_back(uid, name);
		    }
	    }
        cJSON_Delete(root);
        return list;
    }

    optional<std::vector<ListItem>> Paprika3List::list() {
        ESP_LOGD(TAG, "Perform List");
        auto client = initHttpClient();
        esp_http_client_set_method(client, HTTP_METHOD_GET);
        esp_http_client_set_url(client, (this->paprikaRestURLv2 + "sync/groceries/").c_str());
        esp_http_client_set_header(client, "Content-Type", "application/json");
        esp_http_client_set_header(client, "Authorization", ("Bearer " + this->paprikaBearerToken).c_str());

        esp_err_t err = esp_http_client_perform(client);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
            this->status_set_warning();
            cleanHttpClient(client);
            return list();
        }
        int status_code = esp_http_client_get_status_code(client);
        if (status_code != HttpStatus_Ok) {
            ESP_LOGE(TAG, "List failed. HTTP request status code: %d", status_code);
            return optional<std::vector<ListItem>>();
        }
        cleanHttpClient(client);
        
        App.feed_wdt();
        auto list = extractItems(receive_buffer, this->listID);
        ESP_LOGD(TAG, "Performed List well");
        this->status_clear_warning();
        return list;
    }

    char* buildItem(ListItem item, const std::string& listID) {
        if (item.uid.length() == 0) {
            ESP_LOGD(TAG, "Generate ID: %s", item.uid.c_str());
            int range = 999999 - 100000;
            item.uid = std::to_string(esp_random() % range + 100000);
        }

        cJSON *array = cJSON_CreateArray();
        cJSON *object = cJSON_CreateObject();
        cJSON_AddItemToArray(array, object);
        cJSON_AddItemToObject(object, "uid", cJSON_CreateString(item.uid.c_str()));
        cJSON_AddItemToObject(object, "quantity", cJSON_CreateString("1"));
        cJSON_AddItemToObject(object, "list_uid", cJSON_CreateString(listID.c_str()));
        cJSON_AddItemToObject(object, "name", cJSON_CreateString(item.name.c_str()));
        cJSON_AddItemToObject(object, "order_flag", cJSON_CreateNumber(0));
        cJSON_AddItemToObject(object, "purchased", cJSON_CreateBool(false));
        
        char* post_data = cJSON_Print(array);
        cJSON_Delete(array);
        return post_data;
    }

    std::tuple<unsigned char*, int, int>  compressedContent(const char* content) {
        const mz_ulong original_length = strlen(content);
        mz_ulong compressed_len = mz_compressBound(original_length);

        auto compressedContent = new unsigned char[compressed_len];
        const char gzipInit[10] = {0x1F,0x8B,8,0,0,0,0,0,0,0xFF};
        memcpy(compressedContent, gzipInit, 10);

        if (int err = compress(compressedContent + 10, &compressed_len, (unsigned char *)content, original_length) != MZ_OK) {
            delete[] compressedContent;
            return std::make_tuple(nullptr, 0, err);
        }
        
        mz_ulong crc = mz_crc32(MZ_CRC32_INIT, (unsigned char *)content, original_length);
        int footerStart = (int)compressed_len + 10;
        compressedContent[footerStart] = crc & 0xFF;
        compressedContent[footerStart + 1] = (crc >> 8) & 0xFF;
        compressedContent[footerStart + 2] = (crc >> 16) & 0xFF;
        compressedContent[footerStart + 3] = (crc >> 24) & 0xFF;
        compressedContent[footerStart + 4] = original_length & 0xFF;
        compressedContent[footerStart + 5] = (original_length >> 8) & 0xFF;
        compressedContent[footerStart + 6] = (original_length >> 16) & 0xFF;
        compressedContent[footerStart + 7] = (original_length >> 24) & 0xFF;
        compressed_len += 18;

        return std::make_tuple(compressedContent, compressed_len, MZ_OK);
    }

    std::tuple<char*,int> formEncode(const unsigned char* compressedContent, int compressed_len) {
        std::string BOUNDARY = "ea48d9ece76ed1214889ad888a9b16e3";
        const char* part1 = "--ea48d9ece76ed1214889ad888a9b16e3\r\nContent-Disposition: form-data; name=\"data\"; filename=\"data\"\r\n\r\n";
        const char* part2 = "\r\n--ea48d9ece76ed1214889ad888a9b16e3--\r\n";

        int length = strlen(part1) + compressed_len + strlen(part2);
        auto part = new char[length];
        memcpy(part, part1, strlen(part1));
        memcpy(part + strlen(part1), compressedContent, compressed_len);
        memcpy(part + strlen(part1) + compressed_len, part2, strlen(part2));
        return std::make_tuple(part, length);
    }

    void Paprika3List::createItem(const ListItem& item) {
        App.feed_wdt();
        auto post_data = buildItem(item, this->listID);
        ESP_LOGI(TAG, "data: %s", post_data);

        auto [zpkg, compressed_len, err] = compressedContent(post_data);
        if (err != MZ_OK) {
            ESP_LOGE(TAG, "gzip failed, %d", err);
            this->status_set_warning();
            return;
        }

        auto [form, form_length] = formEncode(zpkg, compressed_len);
        delete[] zpkg;

        App.feed_wdt();
        auto client = initHttpClient();
        esp_http_client_set_method(client, HTTP_METHOD_POST);
        esp_http_client_set_url(client, (this->paprikaRestURLv2 + "sync/groceries").c_str());
        esp_http_client_set_header(client, "Authorization", ("Bearer " + this->paprikaBearerToken).c_str());
        esp_http_client_set_header(client, "Accept-Encoding", "gzip, deflate");
        esp_http_client_set_header(client, "Accept", "*/*");
        esp_http_client_set_header(client, "Connection", "keep-alive");
        esp_http_client_set_header(client, "User-Agent", "python-requests/2.31.0");
        esp_http_client_set_header(client, "Content-Type", "multipart/form-data; boundary=ea48d9ece76ed1214889ad888a9b16e3");
        esp_http_client_set_post_field(client, form, form_length);

        esp_err_t httpErr = esp_http_client_perform(client);
        if (httpErr != ESP_OK) {
            ESP_LOGE(TAG, "HTTP POST ITEM failed: %s", esp_err_to_name(httpErr));
            this->status_set_warning();
            delete[] form;
            cleanHttpClient(client);
            return;
        }
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGD(TAG, "Status code %d", status_code);
        if (status_code != HttpStatus_Ok) {
            ESP_LOGE(TAG, "Item creation failed. HTTP request status code: %d", status_code);
            ESP_LOGE(TAG, "Item creation failed. HTTP response: %s", receive_buffer);
            this->status_set_warning();
        }
        cleanHttpClient(client);
        delete[] form;
        this->status_clear_warning();
    }

    void Paprika3List::setEmail(const std::string& email) {
        this->email = email;
    }

    void Paprika3List::setPassword(const std::string& password) {
        this->password = password;
    }

    void Paprika3List::setListID(const std::string& listID) {
        this->listID = listID;
    }

    void Paprika3List::dump_config() {
        ESP_LOGCONFIG(TAG, "Paprika App List:");
        ESP_LOGCONFIG(TAG, "  listID: %s", this->listID.c_str());
    }

    float Paprika3List::get_setup_priority() const {
        return setup_priority::AFTER_CONNECTION;
    }

}
}
