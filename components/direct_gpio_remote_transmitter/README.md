# Direct GPIO Remote Transmitter

An ESP8266 transmitter for a **direct-wired, already-demodulated** remote-control line. It writes the protocol's mark/space levels straight to a GPIO and does not generate a 38 kHz carrier — the typical use case is an ESP spliced into the wire between a device's IR receiver module and its main board, injecting commands as if they came from the original remote.

**Not** for driving an IR LED: an LED needs the modulated carrier this component deliberately omits. Use the stock `remote_transmitter` for that.

## Why not the stock `remote_transmitter`?

On ESP8266, the stock software transmitter wraps the *entire* packet in an interrupt lock. A NEC frame is ~67 ms (longer with `command_repeats`), and the ESP8266 Wi-Fi SDK cannot survive that long without interrupt servicing: its overdue timers then all fire at once inside the level-1 interrupt handler and overrun the soft watchdog. In practice, every transmission risks a crash looking like:

```
Reason: Soft WDT - Level1Int (exccause=4)
PC: pm_send_nullfunc
BT: etharp_output / memp_free / ets_timer_handler_isr ...
```

`power_save_mode: NONE` does not help — the SDK housekeeping happens regardless.

## How it Works

The component keeps interrupts enabled while transmitting, trading perfect timing for Wi-Fi stability, and then claws most of the timing precision back:

- **Micro-locks on edges.** Interrupts are disabled only for the final ~150 µs approach to each GPIO edge, so a short Wi-Fi interrupt cannot land between "target time reached" and the actual pin write. Everything else runs with interrupts open, and the SDK never starves.
- **Durations stretch, never compress.** Every mark/space duration is anchored to the actual GPIO edge. If a Wi-Fi interrupt delays an edge, the following items keep their full nominal length instead of being shortened to catch up with the original packet timeline — a stretched frame may be rejected by the receiver, but it can never turn into a *different* valid frame by squeezed timings.
- **Yield in long gaps.** While waiting out pauses of ≥10 ms (in practice: the gap between repeated frames), the component calls `yield()` so deferred SDK work runs where stretching is harmless. In-frame items never yield.

The remaining trade-off: a Wi-Fi interrupt can still stretch an individual mark/space enough that the receiving device rejects the frame. Protocols with built-in redundancy (NEC transmits the command byte together with its complement) reject such frames rather than mis-decode them, so the worst case is a *lost* transmission, not a wrong command. Pair the transmitter with feedback-and-retry logic if a command must not be lost.

Like the stock software transmitter, sending blocks the main loop for the duration of the frame.

## Limitations

- ESP8266 only (enforced at config validation). On ESP32 the stock `remote_transmitter` uses the RMT hardware peripheral and has none of these problems — use it instead.
- The carrier frequency requested by protocol encoders is ignored; the output is always plain levels (equivalent to `carrier_duty_percent: 100%`).
- The stock `on_transmit` / `on_complete` triggers are not supported; such config is rejected at validation. Wrap the transmit action in a script if you need surrounding automation.

## Example Configuration

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/khmm12/esphome-components.git
      ref: v1.0.0 # or `main` for the latest
    components:
      - direct_gpio_remote_transmitter

direct_gpio_remote_transmitter:
  id: fireplace_ir_transmitter
  pin:
    number: D5
    # NEC mark = LOW on a demodulated line (idle = HIGH).
    inverted: true

button:
  - platform: template
    id: ir_button_power
    on_press:
      # Stock remote_transmitter.* actions work with this transmitter's id.
      - remote_transmitter.transmit_nec:
          address: 0xFE01
          command: 0xEB14
```

Existing `remote_transmitter.transmit_*` actions continue to work. If the configuration contains more than one transmitter, set `transmitter_id: fireplace_ir_transmitter` explicitly in each action.
