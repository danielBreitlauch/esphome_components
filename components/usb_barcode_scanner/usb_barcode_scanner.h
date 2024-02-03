// SPDX-License-Identifier: GPL-3.0-only

#pragma once

//#ifdef USE_ESP32

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/entity_base.h"
#include "esphome/core/helpers.h"

#include "OpenFoodFacts.h"

namespace esphome {
namespace usb_barcode_scanner{

    class USBBarcodeScanner : public Component, public EntityBase {
      protected:
        esp_err_t init_error_ = ESP_OK;
        OpenFoodFacts openFoodFacts;
      public:
        USBBarcodeScanner() {};

  /* setters */
  /* -- image */
  //void set_frame_size(ESP32CameraFrameSize size);
  //void set_drop_size(uint32_t drop_size);
  /* -- framerates */
  //void set_max_update_interval(uint32_t max_update_interval);
  //void set_idle_update_interval(uint32_t idle_update_interval);

        void setup() override;
        void loop() override;
        void dump_config() override;
        float get_setup_priority() const override;
  /* public API (specific) */
  /*void add_image_callback(std::function<void(std::shared_ptr<CameraImage>)> &&f);
  void start_stream(CameraRequester requester);
  void stop_stream(CameraRequester requester);
  void request_image(CameraRequester requester);
  void update_camera_parameters();
*/
};

}  // namespace usb_barcode_scanner
}  // namespace esphome

//#endif
