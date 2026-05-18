import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate
from esphome import automation
from esphome.const import CONF_ID, CONF_TRIGGER_ID

AUTO_LOAD = ["climate"]

fancoil_climate_ns = cg.esphome_ns.namespace("fancoil_climate")

FancoilClimate = fancoil_climate_ns.class_(
    "FancoilClimate", climate.Climate, cg.Component
)

# Our own trigger -- NOT the core climate::ControlTrigger
FancoilControlTrigger = fancoil_climate_ns.class_(
    "FancoilControlTrigger",
    automation.Trigger.template(climate.ClimateCall.operator("ref")),
)

CONF_ON_CONTROL = "on_control"

CONFIG_SCHEMA = climate._CLIMATE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(FancoilClimate),
        cv.Optional(CONF_ON_CONTROL): automation.validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(FancoilControlTrigger),
            }
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)

    for conf in config.get(CONF_ON_CONTROL, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(
            trigger, [(climate.ClimateCall.operator("ref"), "call")], conf
        )
