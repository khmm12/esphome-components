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
}

void DirectGPIORemoteTransmitter::write_edge_(bool level) {
  if (this->target_time_us_ != 0) {
    // Coarse wait with interrupts enabled, so Wi-Fi ISRs stay serviced and
    // their deferred work never piles up like it does under the stock
    // transmitter's whole-frame InterruptLock.
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

void DirectGPIORemoteTransmitter::write_timing_(bool level, uint32_t duration_us) {
  this->write_edge_(level);

  // Anchor every duration to the actual GPIO edge. If a Wi-Fi interrupt made
  // the previous item longer, do not compress this item to catch up with the
  // original packet timeline.
  this->target_time_us_ = micros() + duration_us;
}

void DirectGPIORemoteTransmitter::send_internal(uint32_t send_times, uint32_t send_wait) {
  ESP_LOGD(TAG, "Sending remote code with interrupts enabled");
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
      this->target_time_us_ = micros() + send_wait;
  }
}

}  // namespace direct_gpio_remote_transmitter
}  // namespace esphome
