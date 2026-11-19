#pragma once

#include <cstdint>

namespace trailnexus::can {

enum class HubPowerStatus { waiting, verifying, ready, locked };

struct HubPowerInputs {
  // Valid telemetry for all fitted jacks and the power-path interlock.
  bool feedback_valid;
  // All high-current paths physically isolated during startup selection.
  bool paths_isolated;
  // Cable-side voltage mask. Used to select upstream only when isolated.
  uint8_t powered_jacks;
  // Independent protection/interlock feedback, not inferred from output voltage.
  bool hardware_fault;
  // Negotiation is complete on the selected upstream data link.
  bool upstream_confirmed;
  // Downstream branches whose role and power budget have been qualified.
  uint8_t authorized_outputs;
};

struct HubPowerDecision {
  HubPowerStatus status;
  // Masks are mutually exclusive. Hardware must enforce isolation independently.
  uint8_t upstream_input;
  uint8_t downstream_outputs;
};

// Policy only. Run on one task. The adapter owns debouncing, rail thresholds,
// asynchronous role negotiation, current limits, and hardware reverse blocking.
class HubPowerRoles {
 public:
  explicit HubPowerRoles(uint8_t jacks)
      : valid_jacks_(jacks == 3 ? 0x07 : jacks == 5 ? 0x1F : 0) {}

  HubPowerDecision evaluate(const HubPowerInputs &in) {
    if (locked_ || !valid_jacks_ || !in.feedback_valid || in.hardware_fault ||
        (in.powered_jacks & ~valid_jacks_) || (in.authorized_outputs & ~valid_jacks_))
      return lock();
    if (upstream_ == 0) {
      if (!in.paths_isolated)
        return lock();
      const auto candidates = in.powered_jacks;
      if (!candidates)
        return {HubPowerStatus::waiting, 0, 0};
      if (candidates & (candidates - 1u))
        return lock();
      upstream_ = candidates;
    }

    if (!(in.powered_jacks & upstream_) || (in.authorized_outputs & upstream_))
      return lock();
    if (!in.upstream_confirmed) {
      if (active_)
        return lock();
      return {HubPowerStatus::verifying, upstream_, 0};
    }

    active_ = true;
    return {HubPowerStatus::ready, upstream_, in.authorized_outputs};
  }

  // Reset only after the adapter confirms all high-current paths are isolated.
  bool reset(bool paths_isolated) {
    if (!paths_isolated)
      return false;
    locked_ = false;
    active_ = false;
    upstream_ = 0;
    return true;
  }

 private:
  HubPowerDecision lock() {
    locked_ = true;
    return {HubPowerStatus::locked, 0, 0};
  }

  const uint8_t valid_jacks_;
  uint8_t upstream_{0};
  bool locked_{false};
  bool active_{false};
};

}  // namespace trailnexus::can
