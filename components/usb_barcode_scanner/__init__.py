# SPDX-License-Identifier: MIT

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome import pins
from esphome.const import (
    CONF_FREQUENCY,
    CONF_ID,
    CONF_RESOLUTION,
    CONF_TRIGGER_ID,
)
from esphome.core import CORE, TimePeriod
from esphome.components.esp32 import add_idf_sdkconfig_option
from esphome.components.esp32 import add_idf_component
from esphome.cpp_helpers import setup_entity


usb_barcode_scanner_ns = cg.esphome_ns.namespace('usb_barcode_scanner')
ESP32Camera = usb_barcode_scanner_ns.class_('ESP32Camera', cg.Component)
OpenFoodFacts = usb_barcode_scanner_ns.class_('OpenFoodFacts', cg.Component)


DEPENDENCIES = ["esp32"]

# AUTO_LOAD = ["esp32_camera"]

# frames
CONF_MAX_FRAMERATE = "max_framerate"
CONF_IDLE_FRAMERATE = "idle_framerate"
CONF_DROP_FRAME_SIZE = "drop_frame_size"

CONFIG_SCHEMA = cv.ENTITY_BASE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(ESP32Camera),
        cv.Optional(CONF_MAX_FRAMERATE, default="5 fps"): cv.All(
            cv.framerate, cv.Range(min=0, min_included=False, max=60)
        ),
        cv.Optional(CONF_IDLE_FRAMERATE, default="0.1 fps"): cv.All(
            cv.framerate, cv.Range(min=0, max=1)
        ),
        cv.Optional(CONF_DROP_FRAME_SIZE, default="7000"): cv.All(
            cv.int_range(min=0, max=100000)
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    #await setup_entity(var, config)
    await cg.register_component(var, config)

    #cg.add(var.set_max_update_interval(1000 / config[CONF_MAX_FRAMERATE]))
    #if config[CONF_IDLE_FRAMERATE] == 0:
    #    cg.add(var.set_idle_update_interval(0))
    #else:
    #    cg.add(var.set_idle_update_interval(1000 / config[CONF_IDLE_FRAMERATE]))
    #cg.add(var.set_drop_size(config[CONF_DROP_FRAME_SIZE]))
    #cg.add(var.set_frame_size(config[CONF_RESOLUTION]))

    #cg.add_define("USE_ESP32_CAMERA")

    assert(CORE.using_esp_idf)
    add_idf_component(
            name="usb_host_hid",
            repo="https://github.com/espressif/esp-usb.git",
            path="host/class/hid/usb_host_hid",
            refresh=TimePeriod(minutes=5)
    )
    
    for d, v in {
        #"CONFIG_ESP_SYSTEM_PANIC_PRINT_HALT": True,
        #"CONFIG_RTCIO_SUPPORT_RTC_GPIO_DESC": True,
        "CONFIG_USB_OTG_SUPPORTED": True,
        "CONFIG_SOC_USB_OTG_SUPPORTED": True,
        "CONFIG_SOC_USB_PERIPH_NUM": True,
        "CONFIG_SOC_USB_SERIAL_JTAG_SUPPORTED": True,
        "CONFIG_SOC_USB_PERIPH_NUM": True,
        "CONFIG_SOC_EFUSE_DIS_USB_JTAG": True,
        "CONFIG_ESP_ROM_USB_SERIAL_DEVICE_NUM" : 4,
        "CONFIG_ESP_PHY_ENABLE_USB": True,
        "CONFIG_ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG": True,
        "CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG_ENABLED": True,
        "CONFIG_USB_OTG_SUPPORTED": True,
        "CONFIG_USB_HOST_CONTROL_TRANSFER_MAX_SIZE": 1024,
        "CONFIG_USB_HOST_HW_BUFFER_BIAS_BALANCED": True,
        "CONFIG_USB_HOST_DEBOUNCE_DELAY_MS": 250,
        "CONFIG_USB_HOST_RESET_HOLD_MS": 30,
        "CONFIG_USB_HOST_RESET_RECOVERY_MS": 30,
        "CONFIG_USB_HOST_SET_ADDR_RECOVERY_MS": 10,

        #"CONFIG_SPIRAM_USE_MALLOC": True, # buffers are big, better let everyone allocate PSRAM
        #
        # USB Stream
        #
        #"CONFIG_USB_STREAM_QUICK_START": True,
       # "CONFIG_UVC_GET_DEVICE_DESC": True,
       # "CONFIG_UVC_GET_CONFIG_DESC": True,
       # "CONFIG_UVC_PRINT_DESC": True,
       # "CONFIG_USB_PRE_ALLOC_CTRL_TRANSFER_URB": True,
       # "CONFIG_USB_PROC_TASK_PRIORITY": 2,
        #"CONFIG_USB_PROC_TASK_CORE": 1,
       # "CONFIG_USB_PROC_TASK_STACK_SIZE": 3072,
       # "CONFIG_USB_WAITING_AFTER_CONN_MS": 50,
       # "CONFIG_USB_ENUM_FAILED_RETRY": True,
       # "CONFIG_USB_ENUM_FAILED_RETRY_COUNT": 10,
       # "CONFIG_USB_ENUM_FAILED_RETRY_DELAY_MS": 200,
        #
        # UVC Stream Config
        #
       # "CONFIG_SAMPLE_PROC_TASK_PRIORITY": 0,
       # #"CONFIG_SAMPLE_PROC_TASK_CORE": 0,
       # "CONFIG_SAMPLE_PROC_TASK_STACK_SIZE": 3072,
       # "CONFIG_UVC_PRINT_PROBE_RESULT": True,
       # "CONFIG_UVC_CHECK_BULK_JPEG_HEADER": True,
       # "CONFIG_UVC_DROP_OVERFLOW_FRAME": True,
       # "CONFIG_UVC_DROP_NO_EOF_FRAME": True,
       # "CONFIG_NUM_BULK_STREAM_URBS": 2,
       # "CONFIG_NUM_BULK_BYTES_PER_URB": 2048,
       # "CONFIG_NUM_ISOC_UVC_URBS": 3,
       # "CONFIG_NUM_PACKETS_PER_URB": 4,
        # end of UVC Stream Config
    }.items():
        add_idf_sdkconfig_option(d, v)

    #for conf in config.get(CONF_ON_STREAM_START, []):
    #    trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
    #    await automation.build_automation(trigger, [], conf)

    #for conf in config.get(CONF_ON_STREAM_STOP, []):
    #    trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
    #    await automation.build_automation(trigger, [], conf)
