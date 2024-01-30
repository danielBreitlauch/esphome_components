// SPDX-License-Identifier: GPL-3.0-only

#pragma once

//#ifdef USE_ESP32

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/entity_base.h"
#include "esphome/core/helpers.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "OpenFoodFacts.h"

namespace esphome {
namespace usb_barcode_scanner{

class dummy : public Component {
 public:
  void setup() override { }
  float get_setup_priority() const override { return setup_priority::BUS; }
};

/* ---------------- ESP32Camera class ---------------- */
class ESP32Camera : public Component, public EntityBase {
 public:
  ESP32Camera();

  /* setters */
  /* -- image */
  //void set_frame_size(ESP32CameraFrameSize size);
  //void set_drop_size(uint32_t drop_size);
  /* -- framerates */
  //void set_max_update_interval(uint32_t max_update_interval);
  //void set_idle_update_interval(uint32_t idle_update_interval);

  /* public API (derivated) */
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

 protected:
  //static void framebuffer_task(void *pv);
  esp_err_t init_error_{ESP_OK};
  OpenFoodFacts openFoodFacts;
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
//extern ESP32Camera *global_esp32_camera;

}  // namespace usb_barcode
}  // namespace esphome

//#endif
