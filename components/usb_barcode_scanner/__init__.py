import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_TRIGGER_ID,
)
from esphome.core import CORE, TimePeriod
from esphome.components.esp32 import add_idf_sdkconfig_option
from esphome.components.esp32 import add_idf_component
from esphome.cpp_helpers import setup_entity


# Text sensor with: barcode, food name   
# Paprika3App object with: listUI, email, password
#   
#automation triggers for barcode, food name, added to list

usb_barcode_scanner_ns = cg.esphome_ns.namespace('usb_barcode_scanner')
USBBarcodeScanner = usb_barcode_scanner_ns.class_('USBBarcodeScanner', cg.Component)

DEPENDENCIES = ["esp32", "network"]
AUTO_LOAD = ["psram"]

CODEOWNERS = ["@danielBreitlauch"]

CONF_GET_FOOD_NAME = "get_food_name"
CONF_OPEN_FOOD_FACTS_REGION = "openfoodfacts_region"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_ID): cv.declare_id(USBBarcodeScanner),
    cv.Optional(CONF_GET_FOOD_NAME, default=True): cv.boolean,
    cv.Optional(CONF_OPEN_FOOD_FACTS_REGION, default="world"): cv.string
}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_resolve_food_name(config[CONF_GET_FOOD_NAME]))
    cg.add(var.set_food_region(config[CONF_OPEN_FOOD_FACTS_REGION]))

    #for conf in config.get(CONF_ON_STREAM_START, []):
    #    trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
    #    await automation.build_automation(trigger, [], conf)

    #for conf in config.get(CONF_ON_STREAM_STOP, []):
    #    trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
    #    await automation.build_automation(trigger, [], conf)


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

        "CONFIG_SPIRAM_USE_MALLOC": True,
    }.items():
        add_idf_sdkconfig_option(d, v)
