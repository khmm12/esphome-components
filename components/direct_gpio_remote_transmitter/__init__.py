from esphome import pins
import esphome.codegen as cg
from esphome.components import remote_base
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_PIN

AUTO_LOAD = ["remote_base"]
MULTI_CONF = True

direct_gpio_remote_transmitter_ns = cg.esphome_ns.namespace(
    "direct_gpio_remote_transmitter"
)

DirectGPIORemoteTransmitter = direct_gpio_remote_transmitter_ns.class_(
    "DirectGPIORemoteTransmitter",
    remote_base.RemoteTransmitterBase,
    cg.Component,
)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(DirectGPIORemoteTransmitter),
            cv.Required(CONF_PIN): pins.internal_gpio_output_pin_schema,
        }
    ).extend(cv.COMPONENT_SCHEMA),
    cv.only_on_esp8266,
)


async def to_code(config):
    pin = await cg.gpio_pin_expression(config[CONF_PIN])
    var = cg.new_Pvariable(config[CONF_ID], pin)
    await cg.register_component(var, config)
