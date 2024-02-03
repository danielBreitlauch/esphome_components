#include <algorithm>

#include "esphome/core/application.h"
#include "esphome/core/log.h"

#include "esp_tls.h"
#include "esp_crt_bundle.h"

#include "cJSON.h"

#include "Paprika3List.h"

namespace esphome {
namespace usb_barcode_scanner{

    static const char *TAG = "Paprika3List";

    const int HTTP_OUTPUT_BUFFER = 4096;

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

    Paprika3List::Paprika3List() {
        this->region = "de";
        this->receive_buffer = new char[HTTP_OUTPUT_BUFFER + 1];
        if (receive_buffer == nullptr) {
            ESP_LOGE(TAG, "Failed to allocate memory for buffer");
            ESP_LOGE(TAG, "Available heap: %" PRIu32, esp_get_free_heap_size());
            exit(-1);
        }

        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
        esp_http_client_config_t config = { nullptr };
        #pragma GCC diagnostic pop
        config.method = HTTP_METHOD_GET;
        config.crt_bundle_attach = esp_crt_bundle_attach;
        config.buffer_size = HTTP_OUTPUT_BUFFER;
        config.user_data = receive_buffer;
        config.event_handler = _http_event_handler2;
        config.skip_cert_common_name_check = true;
        config.keep_alive_enable = true;
        config.use_global_ca_store = true;
        config.url = "https://de.openfoodfacts.org/api/v3/product/";
        
        
        this->client = esp_http_client_init(&config);
    }

    Paprika3List::~Paprika3List() {
        esp_http_client_cleanup(client);
        delete[] this->receive_buffer;
    }

    std::string Paprika3List::getNameFromBarcode(std::string barcode) {
        esp_http_client_set_url(client, ("https://" + this->region + ".openfoodfacts.org/api/v3/product/" + barcode + ".json?fields=product_name,brands,abbreviated_product_name").c_str());

        esp_err_t err = esp_http_client_perform(client);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
            return "";
        }
        int status_code = esp_http_client_get_status_code(client);
        if (status_code != HttpStatus_Ok) {
            ESP_LOGE(TAG, "HTTP request status code: %d", status_code);
            return "";
        }

        //ESP_LOGI(TAG, receive_buffer);

        auto root = cJSON_Parse(receive_buffer);
        auto result = cJSON_GetObjectItem(root, "result");
        auto found = cJSON_GetObjectItem(result, "id")->valuestring;
        if (strcmp("product_found", found) != 0) {
            ESP_LOGE(TAG, "Barcode not found: %s", cJSON_Print(root));
            return "";
        }

        auto product = cJSON_GetObjectItem(root, "product");
        auto name = cJSON_GetObjectItem(product, "product_name")->valuestring;
        auto brands = cJSON_GetObjectItem(product, "brands")->valuestring;
        auto answer = std::string(name) + " (" + brands + ")";
        return answer;   
        /*
            return Item(name=answer['product']['product_name'],
                        sub_name=answer['product']['brands'],
                        url=OpenFoodFacts.url(barcode))
        */
    }

    void Paprika3List::set_region(std::string region) {
        this->region = region;
    }
}
}

/*

paprika3_config = {
    "email": "paprika@flying-stampe.de",
    "password": "5ovXZ4zysc41",
    "listID": "8708FA53-B32E-4809-9483-047DC13D4B81-5665-000004B5487830A8" # scans
}


class Paprika3List(ShopList):

    paprikaRestURL = "https://www.paprikaapp.com/api/v2/"
    paprikaBearerToken = ""
    paprikaListUUID = ""

    def __init__(self, config):
        ShopList.__init__(self)
        self.paprikaListUUID = config['listID']
        self.login(config['email'], config['password'])

    def is_meta_item(self, task):
        return False

    def list_tasks(self):
        headers = {
            'Content-Type': "application/json",
            'Authorization': "Bearer %s" % self.paprikaBearerToken
        }

        items = get(self.paprikaRestURL + "sync/groceries/", headers=headers).json()
        return [x for x in items['result'] if x['list_uid'] == self.paprikaListUUID]

    def item_from_task(self, task, with_selects=True):
        if 'quantity' in task and task['quantity'] is not None and is_int(task['quantity']):
            amount = int(task['quantity'])
        else:
            amount = 1

        name = task['name']
        position = name.find(' ')
        if position > 0:
            possible_amount = name[0:position]
            if is_int(possible_amount):
                name = name[position + 1:]
                amount = int(possible_amount)

        if task['purchased']:
            amount = 0

        item = Item(name=name, amount=amount)
        item.select_shop_item(ShopItem(task['uid'], amount, name, None, None, True))
        return item

    def create_item(self, item):
        shop_item = item.selected_shop_item()
        item_str = [
            {
                "uid": str(randrange(1000000)),
                "order_flag": 0,
                "purchased": False,
                "quantity": str(item.amount),
                "list_uid": self.paprikaListUUID,
                "name": str(item.amount) + " " + item.name
            }
        ]
        if shop_item and shop_item.article_id:
            item_str[0]["uid"] = shop_item.article_id

        files = {'data': self.gzip(json.dumps(item_str))}
        post(self.paprikaRestURL + "sync/groceries", files=files, headers={
            'Authorization': "Bearer %s" % self.paprikaBearerToken,
        })

    def update_item(self, task, item, rebuild_notes=False, rebuild_subs=False):
        self.create_item(item)

    
    def gzip(self, content):
        out = io.BytesIO()
        with gzip.GzipFile(fileobj=out, mode='w') as fo:
            fo.write(content.encode())
        bytes_obj = out.getvalue()
        return bytes_obj

    def encode_multipart_formdata(self, fields):
        boundary = "---011000010111000001101001"
        body = (
            "".join("--%s\r\n"
                    "Content-Disposition: form-data; name=\"%s\"\r\n"
                    "\r\n"
                    "%s\r\n" % (boundary, field, value)
                    for field, value in fields.items()) +
            "--%s--\r\n" % boundary
        )

        content_type = "multipart/form-data; boundary=%s" % boundary
        return body, content_type

    def login(self, email, password):
        body, content_type = self.encode_multipart_formdata({
            'email': email,
            'password': password
        })

        response = post(self.paprikaRestURL + "account/login/", data=body, headers={
            'Content-Type': content_type
        })

        if response.status_code == 200:
            json = response.json()
            if json != "":
                self.paprikaBearerToken = json['result']['token']
                return

        print("Wrong credentials")
        exit(1)

*/