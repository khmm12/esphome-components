#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome {
namespace gpio_pwm_binary_sensor {

// Store class for ISR data (no vtables, ISR-safe)
class GPIOPWMBinarySensorStore {
 public:
  void setup(InternalGPIOPin *pin, gpio::InterruptType type, Component *component);

  static void gpio_intr(GPIOPWMBinarySensorStore *arg);

  bool has_triggered() const {
    return this->triggered_;
  }

  bool triggered_state() const {
    return this->triggered_state_;
  }

  void clear_triggered() {
    this->triggered_ = false;
    this->triggered_state_ = false;
  }

 protected:
  ISRInternalGPIOPin isr_pin_;
  volatile bool triggered_{false};
  volatile bool triggered_state_{false};
};

class GPIOPWMBinarySensor : public binary_sensor::BinarySensor, public Component {
 public:
  // No destructor needed: ESPHome components are created at boot and live forever.
  // Interrupts are only detached on reboot when memory is cleared anyway.

  void set_pin(GPIOPin *pin) { pin_ = pin; }
  // ========== INTERNAL METHODS ==========
  // (In most use cases you won't need these)
  /// Setup pin
  void setup() override;
  void dump_config() override;
  /// Hardware priority
  float get_setup_priority() const override;
  /// Check sensor
  void loop() override;

  void set_delayed_off_ms(uint32_t ms) { delayed_off_ms = ms; }
  void set_debounce_ms(uint32_t ms) { debounce_ms = ms; }

protected:
  void set_pending_state(bool new_state);
  void flush_pending_state();

 protected:
  GPIOPin *pin_;
  GPIOPWMBinarySensorStore store_;

  bool use_interrupt_ = true;

  uint32_t last_active_at_ms = 0;

  uint32_t delayed_off_ms = 0;
  uint32_t debounce_ms = 0;

  bool pending_state_ = false;
  bool has_pending_state_ = false;
  uint32_t pending_state_changed_at_ms = 0;

  bool actual_state_ = false;
};

}  // namespace gpio_pwm_binary_sensor
}  // namespace esphome
