from esphome.components import text_sensor
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import (
    CONF_ID,
    CONF_TYPE,
)
from esphome.core import CORE, TimePeriod
from esphome.components.esp32 import add_idf_sdkconfig_option
from esphome.components.esp32 import add_idf_component

usb_barcode_scanner_ns = cg.esphome_ns.namespace('usb_barcode_scanner')
USBBarcodeScanner = usb_barcode_scanner_ns.class_('USBBarcodeScanner', text_sensor.TextSensor, cg.Component)

DEPENDENCIES = ["esp32", "network"]
AUTO_LOAD = ["psram"]
CODEOWNERS = ["@danielBreitlauch"]

CONF_OPEN_FOOD_FACTS_REGION = "openfoodfacts_region"
RESOLV_FOOD_TYPES = {
    "barcode": False,
    "food": True,
}

CONFIG_SCHEMA = cv.All(text_sensor.text_sensor_schema().extend({
    cv.GenerateID(): cv.declare_id(USBBarcodeScanner),
    cv.Optional(CONF_TYPE, default="food"): cv.one_of(*RESOLV_FOOD_TYPES, lower=True),
    cv.Optional(CONF_OPEN_FOOD_FACTS_REGION, default="world"): cv.string
}))


async def to_code(config):
    var = await text_sensor.new_text_sensor(config)
    await cg.register_component(var, config)

    cg.add(var.set_resolve_food_name(RESOLV_FOOD_TYPES[config[CONF_TYPE]]))
    cg.add(var.set_food_region(config[CONF_OPEN_FOOD_FACTS_REGION]))

    #paren = await cg.get_variable(config[CONF_SUN_ID])
    #cg.add(var.set_parent(paren))

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
