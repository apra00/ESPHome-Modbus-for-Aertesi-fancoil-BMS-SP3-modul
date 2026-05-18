import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate
from esphome.automation import validate_automation, build_automation
from esphome.const import CONF_ID, CONF_TRIGGER_ID

DEPENDENCIES = ["climate"]
AUTO_LOAD = ["climate"]

fancoil_climate_ns = cg.esphome_ns.namespace("fancoil_climate")
FancoilClimate = fancoil_climate_ns.class_("FancoilClimate", climate.Climate, cg.Component)

ClimateCallConstRef = climate.ClimateCall.operator("const").operator("ref")

Trigger = cg.esphome_ns.class_("Trigger")

on_control_trigger = fancoil_climate_ns.class_(
    "FancoilClimateControlTrigger",
    Trigger.template(ClimateCallConstRef),
)

CONFIG_SCHEMA = climate._CLIMATE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(FancoilClimate),
        cv.Optional("on_control"): validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(on_control_trigger),
            }
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)

    for conf in config.get("on_control", []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await build_automation(trigger, [(ClimateCallConstRef, "x")], conf)
