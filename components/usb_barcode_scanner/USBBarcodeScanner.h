
#pragma once

//#ifdef USE_ESP32

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/entity_base.h"
#include "esphome/core/helpers.h"

#include "OpenFoodFacts.h"
#include "Scanner.h"

namespace esphome {
namespace usb_barcode_scanner {

    class USBBarcodeScanner : public Component {
    protected:
        esp_err_t init_error_ = ESP_OK;
        OpenFoodFacts openFoodFacts;
        esphome::usb_barcode_scanner::Scanner scanner;
        bool resolveFoodName;
    public:
        USBBarcodeScanner() {};
        void set_food_region(std::string region);
        void set_resolve_food_name(bool doIt);
        void setup() override;
        void loop() override;
        void dump_config() override;
        float get_setup_priority() const override;
};

}  // namespace usb_barcode_scanner
}  // namespace esphome

//#endif
