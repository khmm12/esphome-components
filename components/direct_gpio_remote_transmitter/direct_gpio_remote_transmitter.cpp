#include "direct_gpio_remote_transmitter.h"

#include "esphome/core/application.h"
#include "esphome/core/log.h"

namespace esphome {
namespace direct_gpio_remote_transmitter {

static const char *const TAG = "direct_gpio_remote_transmitter";

void DirectGPIORemoteTransmitter::setup() {
  this->pin_->setup();
  this->pin_->digital_write(false);
}

void DirectGPIORemoteTransmitter::dump_config() {
  ESP_LOGCONFIG(TAG, "Direct GPIO Remote Transmitter (interrupt-friendly):");
  LOG_PIN("  Pin: ", this->pin_);
}

void DirectGPIORemoteTransmitter::wait_for_target_() {
  if (this->target_time_us_ == 0)
    return;

  while (static_cast<int32_t>(this->target_time_us_ - micros()) > 0) {
    // Busy-wait, but intentionally keep interrupts enabled so the ESP8266
    // Wi-Fi SDK can continue servicing its time-critical interrupt path.
  }
}

void DirectGPIORemoteTransmitter::write_timing_(bool level, uint32_t duration_us) {
  this->wait_for_target_();
  this->pin_->digital_write(level);

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
      App.feed_wdt();
    }

    this->wait_for_target_();
    this->pin_->digital_write(false);

    if (repeat + 1 < send_times)
      this->target_time_us_ = micros() + send_wait;
  }
}

}  // namespace direct_gpio_remote_transmitter
}  // namespace esphome
