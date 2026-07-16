#include "gpio_pwm_binary_sensor.h"
#include "esphome/core/log.h"

#include <cinttypes>

namespace esphome {
namespace gpio_pwm_binary_sensor {

static const char *const TAG = "gpio.pwm_binary_sensor";

static const bool ACTIVE = true;

void IRAM_ATTR GPIOPWMBinarySensorStore::gpio_intr(GPIOPWMBinarySensorStore *arg) { arg->triggered_ = true; }

void GPIOPWMBinarySensorStore::setup(InternalGPIOPin *pin) {
  pin->setup();

  // The interrupt fires on the rising edge only, so a trigger by itself means
  // an active pulse was seen — no need to read the pin inside the ISR (a short
  // pulse may already be LOW by the time the ISR runs).
  pin->attach_interrupt(&GPIOPWMBinarySensorStore::gpio_intr, this, gpio::INTERRUPT_RISING_EDGE);
}

void GPIOPWMBinarySensor::setup() {
  this->store_.setup(this->pin_);

  this->actual_state_ = this->pin_->digital_read();
  this->publish_initial_state(this->actual_state_);
}

void GPIOPWMBinarySensor::dump_config() {
  LOG_BINARY_SENSOR("", "GPIO PWM Binary Sensor", this);
  LOG_PIN("  Pin: ", this->pin_);
  ESP_LOGCONFIG(TAG, "  Delayed off: %" PRIu32 "ms", this->delayed_off_ms_);
  ESP_LOGCONFIG(TAG, "  Debounce: %" PRIu32 "ms", this->debounce_ms_);
}

void GPIOPWMBinarySensor::loop() {
  bool has_triggered = this->store_.has_triggered();
  if (has_triggered)
    this->store_.clear_triggered();

  if (has_triggered || this->pin_->digital_read() == ACTIVE) {
    // active
    this->last_active_at_ms_ = millis();
    // publish state active
    this->set_pending_state(ACTIVE);
  } else if (millis() - this->last_active_at_ms_ > this->delayed_off_ms_) {
    // publish state inactive
    this->set_pending_state(!ACTIVE);
  }

  this->flush_pending_state();
}

void GPIOPWMBinarySensor::set_pending_state(bool new_state) {
  if (new_state == this->pending_state_)
    return;  // No change needed

  ESP_LOGD(TAG, "Pending state changed from %s to %s", this->pending_state_ ? "ON" : "OFF", new_state ? "ON" : "OFF");

  this->pending_state_ = new_state;
  this->pending_state_changed_at_ms_ = millis();
  this->has_pending_state_ = true;
}

void GPIOPWMBinarySensor::flush_pending_state() {
  if (!this->has_pending_state_)
    return;  // no pending state to flush

  if (millis() - this->pending_state_changed_at_ms_ < this->debounce_ms_)
    return;  // too early, skip

  if (this->actual_state_ != this->pending_state_) {
    ESP_LOGD(TAG, "Flushing pending state from %s to %s", this->actual_state_ ? "ON" : "OFF",
             this->pending_state_ ? "ON" : "OFF");

    this->actual_state_ = this->pending_state_;
    this->publish_state(this->actual_state_);
  }

  this->has_pending_state_ = false;
}

float GPIOPWMBinarySensor::get_setup_priority() const { return setup_priority::HARDWARE; }

}  // namespace gpio_pwm_binary_sensor
}  // namespace esphome
