import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

# CODEOWNERS = ["@paulschulz"]
# AUTO_LOAD = []
# MULTI_CONF = True

lora_sx126x_ns = cg.esphome_ns.namespace("lora_sx126x")

# empty_component_ns = cg.esphome_ns.namespace('empty_component')
LoraSX126X = lora_sx126x_ns.class_('LoraSX126X', cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(LoraSX126X)
}).extend(cv.COMPONENT_SCHEMA)

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
