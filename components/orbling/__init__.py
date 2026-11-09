import esphome.codegen as cg
from esphome.components import display, time as time_, touchscreen
import esphome.config_validation as cv
from esphome.const import CONF_DISPLAY_ID, CONF_ID, CONF_TIME_ID

CODEOWNERS = ["@clowrey"]
DEPENDENCIES = ["display", "touchscreen", "time"]

CONF_TOUCHSCREEN_ID = "touchscreen_id"
CONF_FALLBACK_TIME_ID = "fallback_time_id"

orbling_ns = cg.esphome_ns.namespace("orbling")
Orbling = orbling_ns.class_("Orbling", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(Orbling),
        cv.Required(CONF_DISPLAY_ID): cv.use_id(display.Display),
        cv.Required(CONF_TOUCHSCREEN_ID): cv.use_id(touchscreen.Touchscreen),
        cv.Required(CONF_TIME_ID): cv.use_id(time_.RealTimeClock),
        # Used while time_id has no valid time yet (e.g. Home Assistant time with no Home Assistant).
        cv.Optional(CONF_FALLBACK_TIME_ID): cv.use_id(time_.RealTimeClock),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_display(await cg.get_variable(config[CONF_DISPLAY_ID])))
    cg.add(var.set_touchscreen(await cg.get_variable(config[CONF_TOUCHSCREEN_ID])))
    cg.add(var.set_time(await cg.get_variable(config[CONF_TIME_ID])))
    if CONF_FALLBACK_TIME_ID in config:
        cg.add(var.set_fallback_time(await cg.get_variable(config[CONF_FALLBACK_TIME_ID])))
