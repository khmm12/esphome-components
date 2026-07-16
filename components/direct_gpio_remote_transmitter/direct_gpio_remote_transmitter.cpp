#include "direct_gpio_remote_transmitter.h"

#include "esphome/core/application.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace direct_gpio_remote_transmitter {

static const char *const TAG = "direct_gpio_remote_transmitter";

// Interrupts are disabled only for this final approach to each edge: long
// enough to keep the edge precise against short Wi-Fi ISRs, short enough that
// the SDK never starves (the stock transmitter's whole-frame lock is what
// crashes the ESP8266 in pm_send_nullfunc).
static const uint32_t LOCKED_TAIL_US = 150;

// Only waits with at least this much time remaining may yield() to the SDK.
// In practice that is the pause between repeated frames, where stretching is
// harmless; in-frame items (<= 9 ms for NEC) never yield, keeping bit timing
// tight.
static const uint32_t YIELD_MIN_REMAINING_US = 10000;

void DirectGPIORemoteTransmitter::setup() {
  this->pin_->setup();
  this->pin_->digital_write(false);
}

void DirectGPIORemoteTransmitter::dump_config() {
  ESP_LOGCONFIG(TAG, "Direct GPIO Remote Transmitter (interrupt-friendly):");
  LOG_PIN("  Pin: ", this->pin_);
  ESP_LOGCONFIG(TAG, "  Carrier: none (direct baseband output)");
}

void DirectGPIORemoteTransmitter::write_edge_(bool level) {
  if (this->target_time_us_ != 0) {
    // Coarse wait with interrupts enabled, so Wi-Fi ISRs stay serviced and
    // their deferred work never piles up like it does under the stock
    // transmitter's whole-frame InterruptLock.
    //
    // The casts on the right-hand constants are load-bearing: without them the
    // signed difference would be promoted to unsigned, an overshoot (negative
    // difference) would compare as a huge positive value, and the loop would
    // hang.
    while (static_cast<int32_t>(this->target_time_us_ - micros()) > static_cast<int32_t>(LOCKED_TAIL_US)) {
      App.feed_wdt();
      if (static_cast<int32_t>(this->target_time_us_ - micros()) > static_cast<int32_t>(YIELD_MIN_REMAINING_US))
        yield();
    }
  }

  // Precise final approach: interrupts are locked for at most ~LOCKED_TAIL_US,
  // so a short ISR can no longer land between "target reached" and the GPIO
  // write and stretch the previous item.
  InterruptLock lock;
  if (this->target_time_us_ != 0) {
    while (static_cast<int32_t>(this->target_time_us_ - micros()) > 0) {
    }
  }
  this->pin_->digital_write(level);
}

void DirectGPIORemoteTransmitter::schedule_target_(uint32_t delay_us) {
  uint32_t target = micros() + delay_us;
  // 0 is the "no pending deadline" sentinel; nudge a real deadline off it so a
  // micros() wrap landing exactly on 0 can't silently skip a wait.
  this->target_time_us_ = (target == 0) ? 1 : target;
}

void DirectGPIORemoteTransmitter::write_timing_(bool level, uint32_t duration_us) {
  this->write_edge_(level);

  // Anchor every duration to the actual GPIO edge. If a Wi-Fi interrupt made
  // the previous item longer, do not compress this item to catch up with the
  // original packet timeline.
  this->schedule_target_(duration_us);
}

void DirectGPIORemoteTransmitter::send_internal(uint32_t send_times, uint32_t send_wait) {
  if (this->temp_.get_data().empty()) {
    ESP_LOGW(TAG, "Empty transmit data, nothing sent");
    return;
  }

  ESP_LOGD(TAG, "Sending remote code");
  this->target_time_us_ = 0;

  for (uint32_t repeat = 0; repeat < send_times; repeat++) {
    for (int32_t item : this->temp_.get_data()) {
      if (item > 0) {
        this->write_timing_(true, static_cast<uint32_t>(item));
      } else {
        this->write_timing_(false, static_cast<uint32_t>(-item));
      }
    }

    this->write_edge_(false);  // wait out the last item, then release the line

    if (repeat + 1 < send_times)
      this->schedule_target_(send_wait);
  }
}

}  // namespace direct_gpio_remote_transmitter
}  // namespace esphome
