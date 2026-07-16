# My ESPHome Custom Components

[![CI](https://github.com/khmm12/esphome-components/actions/workflows/ci.yaml/badge.svg)](https://github.com/khmm12/esphome-components/actions/workflows/ci.yaml)

Welcome to my collection of ESPHome external components. These are the building blocks I develop for my own devices - sensors, switches and other creative integrations that didn't fit neatly into the stock ESPHome feature set.

Don't be surprised if you spot the occasional bit of humour or an optimistic nod toward the future - these projects are experiments and built for personal needs more than finished products.

## Components

| Component | What it does | Platforms |
| --- | --- | --- |
| [gpio_pwm_binary_sensor](components/gpio_pwm_binary_sensor/) | Turns a fast-changing PWM input (e.g. a "breathing" indicator LED) into a stable ON/OFF binary sensor, using interrupts so even the shortest pulses are never missed | ESP8266, ESP32 |
| [direct_gpio_remote_transmitter](components/direct_gpio_remote_transmitter/) | Remote-control transmitter for a direct-wired, already-demodulated line; keeps interrupts enabled during transmission, avoiding the ESP8266 Wi-Fi crash caused by the stock `remote_transmitter` | ESP8266 |

Each component's README covers its configuration, a ready-to-paste `external_components` snippet, and the story behind it.

## Disclaimer

These components are designed for my own devices and may not work out‑of‑the‑box on other hardware. Feel free to explore, adapt and improve them, but please understand that they are provided "as is" and may change without notice.
