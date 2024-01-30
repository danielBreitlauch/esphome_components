
#pragma once

namespace esphome {
namespace usb_barcode_scanner{

class OpenFoodFacts {
public:
    bool getNameFromBarcode(unsigned char* barcode, unsigned char* answer);
};

}
}
