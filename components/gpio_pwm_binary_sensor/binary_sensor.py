from esphome import pins
import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import CONF_ALLOW_OTHER_USES, CONF_NUMBER, CONF_PIN
from esphome.core import CORE

gpio_pwm_binary_sensor_ns = cg.esphome_ns.namespace("gpio_pwm_binary_sensor")

GPIOPWMBinarySensor = gpio_pwm_binary_sensor_ns.class_(
    "GPIOPWMBinarySensor", binary_sensor.BinarySensor, cg.Component
)

CONF_DELAYED_OFF = "delayed_off"
CONF_DEBOUNCE = "debounce"


def _validate_pin_supports_interrupts(config):
    pin = config[CONF_PIN]
    if pin.get(CONF_ALLOW_OTHER_USES, False):
        raise cv.Invalid(
            "This component requires exclusive use of the pin for interrupts, "
            "so allow_other_uses is not supported."
        )
    # GPIO16 on ESP8266 is a special pin that doesn't support interrupts
    # through the Arduino attachInterrupt() function.
    if CORE.is_esp8266 and pin[CONF_NUMBER] == 16:
        raise cv.Invalid("GPIO16 on ESP8266 doesn't support interrupts.")
    return config


CONFIG_SCHEMA = cv.All(
    binary_sensor.binary_sensor_schema(GPIOPWMBinarySensor)
    .extend(
        {
            cv.Required(CONF_PIN): pins.internal_gpio_input_pin_schema,
            cv.Optional(
                CONF_DELAYED_OFF, default="20ms"
            ): cv.positive_time_period_milliseconds,
            cv.Optional(
                CONF_DEBOUNCE, default="20ms"
            ): cv.positive_time_period_milliseconds,
        }
    )
    .extend(cv.COMPONENT_SCHEMA),
    _validate_pin_supports_interrupts,
)


async def to_code(config):
    var = await binary_sensor.new_binary_sensor(config)
    await cg.register_component(var, config)

    pin = await cg.gpio_pin_expression(config[CONF_PIN])
    cg.add(var.set_pin(pin))

    cg.add(var.set_delayed_off_ms(config[CONF_DELAYED_OFF]))
    cg.add(var.set_debounce_ms(config[CONF_DEBOUNCE]))
