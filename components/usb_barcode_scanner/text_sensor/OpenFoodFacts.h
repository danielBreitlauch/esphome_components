
#pragma once

#include <string>
#include "esp_http_client.h"

namespace esphome {
namespace usb_barcode_scanner{

struct Context {
    std::string barcode = "";
};

enum Func {
    NONE,
    GET_LIST_ITEM_FROM_BARCODE
};

struct AsyncState {
    Func function = NONE;
    bool exit = false;
    Context data;
};

class OpenFoodFacts {
public:
    OpenFoodFacts();
    ~OpenFoodFacts();
    optional<std::string> getListItemFromBarcode(const std::string& barcode);
    void set_region(std::string region);
    std::string get_region();
    AsyncState state;
    friend esp_err_t _http_event_handler(esp_http_client_event_t *evt);
protected:
    esp_http_client_handle_t client;
    esp_http_client_handle_t initHttpClient();
    void cleanHttpClient(esp_http_client_handle_t client);
    char* receive_buffer;
    int bytes_read = 0;
    std::string region;
};

}
}
