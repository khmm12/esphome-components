# GPIO PWM Binary Sensor

This component is designed for handling PWM signals or other fast-changing input states as a binary state. It is particularly useful for detecting the operational status of a device by monitoring an indicator LED that uses PWM for brightness control (e.g., "breathing" effects.).

A common use case is monitoring a device's power status via its backlight. For example, an electric fireplace might use PWM to control its backlight brightness. A standard binary sensor would flicker ON and OFF rapidly at low brightness levels. This component smooths that PWM input into a stable ON/OFF state, correctly reporting that the fireplace is on even when the backlight is dim.

## How it Works

The component uses a hybrid approach, combining GPIO interrupts and polling, to reliably detect both fast-changing PWM signals and steady `HIGH` states.

- **Interrupts for PWM:** It uses GPIO interrupts to efficiently capture the RISING edge of a PWM signal. This is ideal for "breathing" LEDs or other low-duty-cycle signals, as it reacts instantly to pulses.

- **Polling for Steady State:** In addition to interrupts, the component also polls the GPIO pin's state in every loop cycle. This is a fallback mechanism that ensures a constantly `HIGH` signal (like a 100% duty cycle PWM for full brightness) is correctly detected. This prevents the sensor from getting stuck in an `OFF` state if the device powers on with the signal already high, a scenario where no initial interrupt would occur.

Whenever the signal is detected as active (either through an interrupt or by polling in the main loop), an internal timer is reset, and the sensor's state is set to `ON`. The sensor will remain `ON` as long as new active states are detected within the `delayed_off` period.

If no active signal is detected for a duration longer than `delayed_off`, the sensor's state changes to `OFF`. This logic effectively smooths a fast-pulsing signal into a stable binary state, correctly handling both PWM and steady-on signals.

The `debounce` option provides an additional layer of filtering, preventing the state from flapping rapidly during state transitions (e.g., when a device is turning off) or when the microcontroller is busy.

## Example Configuration

```yaml
binary_sensor:
  - platform: gpio_pwm_binary_sensor
    id: power_feedback
    internal: true
    pin:
      # The GPIO pin connected to the PWM signal source (e.g., backlight LED).
      number: D3
      # Invert the logic if your signal is active-low.
      # For example, if monitoring an LED that is ON when the pin is LOW.
      inverted: true
      # Use INPUT_PULLUP or INPUT_PULLDOWN to ensure the pin is in a stable
      # state when the signal is not being driven.
      mode: INPUT_PULLUP
    # Keep the sensor ON for this duration after the last pulse.
    # This prevents the sensor from flapping OFF between PWM pulses.
    # It should be longer than the time between individual PWM pulses.
    delayed_off: 20ms
    # Debounce time to filter out noise and prevent state flapping.
    debounce: 20ms
```
