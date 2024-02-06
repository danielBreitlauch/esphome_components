
#pragma once

#include <string>
#include "esp_http_client.h"

#include "Paprika3List.h"

namespace esphome {
namespace usb_barcode_scanner{

class OpenFoodFacts {
public:
    OpenFoodFacts();
    ~OpenFoodFacts();
    optional<ListItem> getListItemFromBarcode(std::string barcode);
    void set_region(std::string region);
protected:
    esp_http_client_handle_t initHttpClient();
    void cleanHttpClient(esp_http_client_handle_t client);
    char* receive_buffer;
    std::string region;
};

}
}
