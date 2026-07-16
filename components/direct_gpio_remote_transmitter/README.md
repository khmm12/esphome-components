# Direct GPIO Remote Transmitter

Experimental ESP8266 transmitter for a direct-wired, already-demodulated IR
input. It outputs the protocol's mark/space levels directly and does not
generate a carrier.

The stock ESPHome software `remote_transmitter` holds a global interrupt lock
for an entire packet. This component keeps interrupts enabled to test and avoid
ESP8266 Wi-Fi starvation. Wi-Fi interrupts can lengthen individual timings, so
the receiving appliance must tolerate some jitter. This is not intended for an
IR LED that requires a 38 kHz carrier.

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/khmm12/esphome-components.git
      ref: main
    components:
      - direct_gpio_remote_transmitter

direct_gpio_remote_transmitter:
  id: fireplace_ir_transmitter
  pin:
    number: D5
    inverted: true
```

Existing `remote_transmitter.transmit_nec` actions continue to work. If the
configuration contains more than one transmitter, set
`transmitter_id: fireplace_ir_transmitter` explicitly in each action.
