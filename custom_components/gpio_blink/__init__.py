import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

gpio_blink_ns = cg.esphome_ns.namespace('gpio_blink')
GPIOBlink = gpio_blink_ns.class_('GPIOBlink', cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(GPIOBlink)
}).extend(cv.COMPONENT_SCHEMA)

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
