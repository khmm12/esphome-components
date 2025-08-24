#include "gpio_pwm_binary_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace gpio_pwm_binary_sensor {

static const char *const TAG = "gpio.pwm_binary_sensor";

static const gpio::InterruptType INTERRUPT_TYPE = gpio::INTERRUPT_RISING_EDGE;
static const bool ACTIVE = true;

void IRAM_ATTR GPIOPWMBinarySensorStore::gpio_intr(GPIOPWMBinarySensorStore *arg) {
  arg->triggered_state_ = arg->isr_pin_.digital_read();
  arg->triggered_ = true;
}

void GPIOPWMBinarySensorStore::setup(InternalGPIOPin *pin, gpio::InterruptType type, Component *component) {
  pin->setup();
  this->isr_pin_ = pin->to_isr();

  // Attach interrupt - from this point on, any changes will be caught by the interrupt
  pin->attach_interrupt(&GPIOPWMBinarySensorStore::gpio_intr, this, type);
}

void GPIOPWMBinarySensor::setup() {
  if (!this->pin_->is_internal()) {
    ESP_LOGE(TAG, "GPIO is not internal. Please configure the GPIO as internal.");
  } else {
    auto *internal_pin = static_cast<InternalGPIOPin *>(this->pin_);
    this->store_.setup(internal_pin, INTERRUPT_TYPE, this);
    this->is_interrupts_configured_ = true;
  }

  this->actual_state_ = this->pin_->digital_read();
  this->publish_initial_state(this->actual_state_);
}

void GPIOPWMBinarySensor::dump_config() {
  LOG_BINARY_SENSOR("", "GPIO PWM Binary Sensor", this);
  LOG_PIN("  Pin: ", this->pin_);
}

void GPIOPWMBinarySensor::loop() {
  bool has_triggered = false;

  if (!this->is_interrupts_configured_ && this->store_.has_triggered()) {
    has_triggered = this->store_.triggered_state() == ACTIVE; // Ignore anomalies
    this->store_.clear_triggered();
  }

  if (has_triggered || this->pin_->digital_read() == ACTIVE) {
    // active
    this->last_active_at_ms = millis();
    // publish state active
    this->set_pending_state(ACTIVE);
  } else if (millis() - this->last_active_at_ms > this->delayed_off_ms) {
    // publish state inactive
    this->set_pending_state(!ACTIVE);
  }

  this->flush_pending_state();
}

void GPIOPWMBinarySensor::set_pending_state(bool new_state) {
  if (new_state == this->pending_state_) return; // No change needed

  ESP_LOGD(TAG, "Pending state changed from %s to %s", this->pending_state_? "ON" : "OFF", new_state ? "ON" : "OFF");

  this->pending_state_ = new_state;
  this->pending_state_changed_at_ms = millis();
  this->has_pending_state_ = true;
}

void GPIOPWMBinarySensor::flush_pending_state() {
  if (!this->has_pending_state_) return;

  if (millis() - this->pending_state_changed_at_ms < this->debounce_ms) return; // too early, skip

  if (this->actual_state_ != this->pending_state_) {
    ESP_LOGD(TAG, "Flushing pending state from %s to %s", this->actual_state_ ? "ON" : "OFF", this->pending_state_? "ON" : "OFF");

    this->actual_state_ = this->pending_state_;
    this->publish_state(this->actual_state_);
  }

  this->has_pending_state_ = false;
}

float GPIOPWMBinarySensor::get_setup_priority() const { return setup_priority::HARDWARE; }

}  // namespace gpio_pwm_binary_sensor
}  // namespace esphome
