#pragma once

#include <cstdint>
#include <optional>

namespace trailnexus::can {

constexpr uint8_t default_core_ports = 4;
enum class TerminationMode { settling, idle, one_run, two_runs, unsupported, sense_fault };

struct TerminationDecision {
  TerminationMode mode;
  // No value means hold the previous switch state and inhibit transmission.
  std::optional<bool> connect_core_terminator;
  bool can_operate;
};

// One bit per physical connector. Invalid or unsettled inputs inhibit CAN.
// The adapter owns sense electronics, switch actuation, and contact feedback.
class CoreTermination {
 public:
  TerminationDecision update(uint8_t ports, uint8_t present, uint8_t valid,
                             uint32_t now, uint32_t settle_ms, bool terminator_connected) {
    if (ports < 2 || ports > 8 || settle_ms == 0) {
      seen_ = false;
      return {TerminationMode::sense_fault, std::nullopt, false};
    }
    const auto mask = static_cast<uint8_t>((1u << ports) - 1u);
    if ((valid & mask) != mask || (present & mask) != present) {
      seen_ = false;
      return {TerminationMode::sense_fault, std::nullopt, false};
    }
    if (!seen_ || present != last_ || ports != ports_) {
      seen_ = true;
      last_ = present;
      ports_ = ports;
      changed_at_ = now;
    }
    if (now - changed_at_ < settle_ms)
      return {TerminationMode::settling, std::nullopt, false};

    uint8_t runs = 0;
    for (auto bits = present; bits != 0; bits >>= 1)
      runs += bits & 1;
    switch (runs) {
      case 0: return {TerminationMode::idle, false, false};
      case 1: return {TerminationMode::one_run, true, terminator_connected};
      case 2: return {TerminationMode::two_runs, false, !terminator_connected};
      default: return {TerminationMode::unsupported, std::nullopt, false};
    }
  }

 private:
  uint32_t changed_at_{0};
  uint8_t last_{0};
  uint8_t ports_{0};
  bool seen_{false};
};

// Wire this rule in hardware so the endpoint decision does not depend on
// module firmware booting or on a working CAN bus.
constexpr bool module_termination(bool downstream_connected) {
  return !downstream_connected;
}

}  // namespace trailnexus::can
