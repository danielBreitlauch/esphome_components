
#pragma once

//#ifdef USE_ESP32

#include "esphome/core/helpers.h"
#include <string>

namespace esphome {
namespace usb_barcode_scanner {

class Scanner {
protected:
public:
      Scanner();
      optional<std::string> barcode();
};

}  // namespace usb_barcode_scanner
}  // namespace esphome

//#endif
