#pragma once

#include "esphome/components/remote_base/remote_base.h"
#include "esphome/core/component.h"

namespace esphome {
namespace direct_gpio_remote_transmitter {

// Experimental transmitter for a direct-wired, already-demodulated IR line.
// Unlike ESPHome's software remote_transmitter on ESP8266, it deliberately
// leaves interrupts enabled while timing the complete packet.
class DirectGPIORemoteTransmitter final : public remote_base::RemoteTransmitterBase, public Component {
 public:
  explicit DirectGPIORemoteTransmitter(InternalGPIOPin *pin) : remote_base::RemoteTransmitterBase(pin) {}

  void setup() override;
  void dump_config() override;

  // Set up after the receiver so one physical signal can be received and then
  // retransmitted by an automation during normal operation.
  float get_setup_priority() const override { return setup_priority::DATA - 1; }

 protected:
  void send_internal(uint32_t send_times, uint32_t send_wait) override;
  void wait_for_target_();
  void write_timing_(bool level, uint32_t duration_us);

  uint32_t target_time_us_{0};
};

}  // namespace direct_gpio_remote_transmitter
}  // namespace esphome
