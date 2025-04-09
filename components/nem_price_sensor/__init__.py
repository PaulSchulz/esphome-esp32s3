# This file is required to register the component

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_UPDATE_INTERVAL

nem_price_sensor_ns = cg.esphome_ns.namespace("nem_price_sensor")
NEMPriceSensor = nem_price_sensor_ns.class_("NEMPriceSensor", cg.Component)

CONF_URL = "url"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(NEMPriceSensor),
        cv.Required(CONF_URL): cv.url,
        cv.Optional(CONF_UPDATE_INTERVAL, default="60s"): cv.update_interval,
    }
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_url(config[CONF_URL]))
