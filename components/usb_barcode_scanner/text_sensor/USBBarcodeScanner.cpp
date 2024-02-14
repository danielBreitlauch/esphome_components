
#ifdef USE_ESP32

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
    bool ok = scanner.setup();
    if (!ok) {
        mark_failed();
    }
}

void USBBarcodeScanner::dump_config() {
    ESP_LOGCONFIG(TAG, "USB Barcode Scanner:");
    ESP_LOGCONFIG(TAG, "  resolveFoodName: %s", this->resolveFoodName ? "true" : "false");
    ESP_LOGCONFIG(TAG, "  openFoodFacts region: %s", this->openFoodFacts.get_region().c_str());
  
    if (this->is_failed()) {
        ESP_LOGE(TAG, "  Setup Failed: %s", esp_err_to_name(this->init_error_));
        return;
    }
}

void USBBarcodeScanner::loop() {
    if (openFoodFacts.state.function != NONE) {
        goto RETURN_SPOT;
    }
    {
    auto optionalBarcode = scanner.barcode();
    if (optionalBarcode.has_value()) {
        this->foundBarcode = optionalBarcode.value();
        ESP_LOGI(TAG, "Barcode: %s", this->foundBarcode.c_str());
        return;
    }
    }

    if (this->foundBarcode != "") {
        if (this->resolveFoodName) {
            RETURN_SPOT:
            optional<std::string> item = openFoodFacts.getListItemFromBarcode(this->foundBarcode);
            if (openFoodFacts.state.exit) {
                return;
            }
            if (item.has_value()) {
                ESP_LOGI(TAG, "Name: %s", item->c_str());
                this->publish_state(*item);
            } else {
                ESP_LOGE(TAG, "Error retrieving name");
            }
        } else {
            this->publish_state(this->foundBarcode);
        }
        this->foundBarcode = "";
    }
}

float USBBarcodeScanner::get_setup_priority() const { return setup_priority::AFTER_CONNECTION; }

void USBBarcodeScanner::set_food_region(std::string region) {
    this->openFoodFacts.set_region(region);
}

void USBBarcodeScanner::set_resolve_food_name(bool doIt) {
    this->resolveFoodName = doIt;
}

}  // namespace usb_barcode_scanner
}  // namespace esphome

#endif
