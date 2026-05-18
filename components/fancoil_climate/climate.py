import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate
from esphome.const import CONF_ID, CONF_LAMBDA

AUTO_LOAD = ["climate"]

fancoil_climate_ns = cg.esphome_ns.namespace("fancoil_climate")

FancoilClimate = fancoil_climate_ns.class_(
    "FancoilClimate", climate.Climate, cg.Component
)

# ClimateCall reference type used as the lambda parameter
climate_call_ref = climate.ClimateCall.operator("ref")

CONFIG_SCHEMA = climate._CLIMATE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(FancoilClimate),
        # User writes a lambda: block; receives `call` (ClimateCall &)
        cv.Optional(CONF_LAMBDA): cv.returning_lambda,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)

    if CONF_LAMBDA in config:
        # process_lambda compiles the YAML lambda body into a C++ lambda.
        # Parameters: [(type, name), ...]  -- `call` available inside the body.
        control_lambda = await cg.process_lambda(
            config[CONF_LAMBDA],
            [(climate_call_ref, "call")],
            return_type=cg.void,
        )
        cg.add(var.set_control_callback(control_lambda))
