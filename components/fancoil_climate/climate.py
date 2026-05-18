import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate
from esphome.const import CONF_ID

AUTO_LOAD = ["climate"]

fancoil_climate_ns = cg.esphome_ns.namespace("fancoil_climate")

FancoilClimate = fancoil_climate_ns.class_(
    "FancoilClimate", climate.Climate, cg.Component
)

# Inherit the full base CLIMATE_SCHEMA unchanged -- on_control, on_state,
# visual, etc. are all already defined there. We add nothing extra.
CONFIG_SCHEMA = climate.CLIMATE_SCHEMA.extend(
    {cv.GenerateID(): cv.declare_id(FancoilClimate)}
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)
