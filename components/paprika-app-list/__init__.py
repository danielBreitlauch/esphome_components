import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.const import (
    CONF_ID,
    CONF_FORMAT,
    CONF_PASSWORD
)
import re

usb_barcode_scanner_ns = cg.esphome_ns.namespace('usb_barcode_scanner')
Paprika3List = usb_barcode_scanner_ns.class_('Paprika3List', cg.Component)
Paprika3ListAddAction = usb_barcode_scanner_ns.class_("Paprika3ListAddAction", automation.Action)


DEPENDENCIES = ["esp32", "network"]
AUTO_LOAD = ["psram"]

CODEOWNERS = ["@danielBreitlauch"]

CONF_EMAIL = "email"
CONF_LIST_ID = "listID"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_ID): cv.declare_id(Paprika3List),
    cv.Required(CONF_EMAIL): cv.string,
    cv.Required(CONF_PASSWORD): cv.string,
    cv.Required(CONF_LIST_ID): cv.string,
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.setEmail(config[CONF_EMAIL]))
    cg.add(var.setPassword(config[CONF_PASSWORD]))
    cg.add(var.setListID(config[CONF_LIST_ID]))

    cg.add_library("HTTPClient", None)


def validate_format(format):
    #if re.search(r"^.*\%s.*$", format) is None:
    if re.search(r"^[^%]*%s[^%]*$", format) is None:
        raise cv.Invalid(f"{CONF_FORMAT}: has to specify a printf-like format string specifying exactly one %s type, '{format}' provided")

    return format

PAPRIKA3LIST_ADD_ACTION_SCHEMA = cv.Schema({
        cv.GenerateID(): cv.use_id(Paprika3List),
        cv.Optional(CONF_FORMAT, default="%s"): cv.All(cv.string_strict, validate_format)
    })

@automation.register_action("paprika-app-list.add", Paprika3ListAddAction, PAPRIKA3LIST_ADD_ACTION_SCHEMA)
async def http_request_action_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)

    cg.add(var.set_format(config[CONF_FORMAT]))
    return var

