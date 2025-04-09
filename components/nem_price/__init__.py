# This file is required to register the component

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID, CONF_UPDATE_INTERVAL

DEPENDENCIES = ["sensor","http_request"]

nem_price_ns = cg.esphome_ns.namespace("nem_price")
NEMPrice = nem_price_ns.class_("NEMPrice", sensor.Sensor, cg.PollingComponent)

CONF_URL    = "url"
CONF_REGION = "region"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(NEMPrice),
    cv.Required(CONF_URL): cv.url,
    cv.Optional(CONF_UPDATE_INTERVAL, default="60s"): cv.update_interval,
    cv.Optional(CONF_REGION, default="NSW1"): cv.string,
     })

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_url(config[CONF_URL]))
    cg.add(var.set_region(config[CONF_REGION]))
