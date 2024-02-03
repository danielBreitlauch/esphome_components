// SPDX-License-Identifier: GPL-3.0-only

//#ifdef USE_ESP32 #TODO: remove

#include "esphome/core/log.h"

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/entity_base.h"
#include "esphome/core/helpers.h"

#include <string>

#include "USBBarcodeScanner.h"


namespace esphome {
namespace usb_barcode_scanner {

static const char *const TAG = "USBBarcodeScanner";

void USBBarcodeScanner::setup() {
}

void USBBarcodeScanner::dump_config() {
  ESP_LOGCONFIG(TAG, "USB Barcode Scanner:");
  ESP_LOGCONFIG(TAG, "  Name: %s", this->name_.c_str());
  
  if (this->is_failed()) {
    ESP_LOGE(TAG, "  Setup Failed: %s", esp_err_to_name(this->init_error_));
    return;
  }
}

void USBBarcodeScanner::loop() {
    auto optionalBarcode = scanner.barcode();
    if (optionalBarcode.has_value()) {
        auto barcode = optionalBarcode.value();
        ESP_LOGI(TAG, "Barcode: %s", barcode.c_str());
        std::string answer = openFoodFacts.getNameFromBarcode("4047247424301");
        //std::string answer = openFoodFacts.getNameFromBarcode(barcode);
        ESP_LOGI(TAG, "Name: %s", answer.c_str());
    }
}

float USBBarcodeScanner::get_setup_priority() const { return setup_priority::AFTER_CONNECTION; }

}  // namespace usb_barcode_scanner
}  // namespace esphome

//#endif
