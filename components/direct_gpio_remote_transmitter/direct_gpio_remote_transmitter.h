#pragma once

#include "esphome/components/remote_base/remote_base.h"
#include "esphome/core/component.h"

namespace esphome {
namespace direct_gpio_remote_transmitter {

// Transmitter for a direct-wired, already-demodulated IR line (no carrier).
// Unlike ESPHome's software remote_transmitter on ESP8266, which locks
// interrupts for the whole frame and starves the Wi-Fi SDK, it keeps
// interrupts enabled for the bulk of each wait and locks them only for a
// short final approach to each edge.
class DirectGPIORemoteTransmitter final : public remote_base::RemoteTransmitterBase, public Component {
 public:
  explicit DirectGPIORemoteTransmitter(InternalGPIOPin *pin) : remote_base::RemoteTransmitterBase(pin) {}

  void setup() override;
  void dump_config() override;

 protected:
  void send_internal(uint32_t send_times, uint32_t send_wait) override;
  void write_edge_(bool level);
  void write_timing_(bool level, uint32_t duration_us);
  void schedule_target_(uint32_t delay_us);

  uint32_t target_time_us_{0};  // 0 = no pending deadline (sentinel)
};

}  // namespace direct_gpio_remote_transmitter
}  // namespace esphome
