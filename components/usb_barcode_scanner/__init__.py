# SPDX-License-Identifier: MIT

import esphome.codegen as cg
import esphome.config_validation as cv
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
USBBarcodeScanner = usb_barcode_scanner_ns.class_('USBBarcodeScanner', cg.Component)

DEPENDENCIES = ["esp32", "network"]
AUTO_LOAD = ["json"]

# frames
CONF_MAX_FRAMERATE = "max_framerate"
CONF_IDLE_FRAMERATE = "idle_framerate"
CONF_DROP_FRAME_SIZE = "drop_frame_size"

CONFIG_SCHEMA = cv.ENTITY_BASE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(USBBarcodeScanner),
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
    }.items():
        add_idf_sdkconfig_option(d, v)

    #for conf in config.get(CONF_ON_STREAM_START, []):
    #    trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
    #    await automation.build_automation(trigger, [], conf)

    #for conf in config.get(CONF_ON_STREAM_STOP, []):
    #    trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
    #    await automation.build_automation(trigger, [], conf)
