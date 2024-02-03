
#pragma once

#include <string>
#include "esp_http_client.h"

namespace esphome {
namespace usb_barcode_scanner{

class Paprika3List {
public:
    Paprika3List();
    ~Paprika3List();
    std::string getNameFromBarcode(std::string barcode);
    void set_region(std::string region);
protected:
    esp_http_client_handle_t client;
    char* receive_buffer;
    std::string region;
};

}
}
