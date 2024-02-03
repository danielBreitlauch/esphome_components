
#pragma once

#include <string>
#include "esp_http_client.h"

//#include "esphome/components/json/json_util.h"



namespace esphome {
namespace usb_barcode_scanner{

class OpenFoodFacts {
public:
    OpenFoodFacts();
    ~OpenFoodFacts();
    std::string getNameFromBarcode(std::string barcode);
protected:
    esp_http_client_handle_t client;
    char* receive_buffer;
};

}
}
