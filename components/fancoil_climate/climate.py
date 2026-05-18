import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate
from esphome.const import CONF_ID

DEPENDENCIES = ["climate"]
AUTO_LOAD = ["climate"]

fancoil_climate_ns = cg.esphome_ns.namespace("fancoil_climate")
FancoilClimate = fancoil_climate_ns.class_("FancoilClimate", climate.Climate, cg.Component)

CONFIG_SCHEMA = climate._CLIMATE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(FancoilClimate),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)
