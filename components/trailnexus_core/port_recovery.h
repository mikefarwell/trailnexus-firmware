#pragma once

#include <array>
#include <cstdint>

namespace trailnexus {

enum class PortHealth { ready, main_unavailable, fault, unknown };
enum class Discovery { pending, recovered, failed };
enum class RecoveryEvent { attempt, recovered, unavailable, fault, control_failed, exhausted };
enum class RecoveryStart { started, busy, unsupported, invalid, locked };

struct RecoveryTiming {
  uint32_t off_ms;
  uint32_t startup_ms;
  uint32_t discovery_ms;
  uint8_t max_attempts;
};

// Implement against a qualified hardware revision and the owning DI2C router.
// Every operation must be nonblocking. False means the requested state is
// unconfirmed. Hardware must independently block power in USB-only operation.
class PortRecoveryControl {
 public:
  virtual ~PortRecoveryControl() = default;
  virtual bool supports(uint8_t port) const = 0;
  virtual PortHealth health(uint8_t port) = 0;
  virtual void inhibit_commands(uint8_t port) = 0;
  virtual bool isolate_data(uint8_t port, bool isolate) = 0;
  virtual bool set_power(uint8_t port, bool enabled) = 0;
  virtual bool begin_discovery(uint8_t port) = 0;
  virtual Discovery poll_discovery(uint8_t port) = 0;
  // Cancel an outstanding scan and invalidate any partially recovered routes.
  virtual void cancel_discovery(uint8_t port) = 0;
  virtual void report(uint8_t port, uint8_t attempt, RecoveryEvent event) = 0;
};

class PortRecovery {
 public:
  // Call only after bridge-level recovery has been attempted or ruled out.
  // Ports are numbered 1–8. The control object must outlive the sequence.
  RecoveryStart start(PortRecoveryControl &control, uint8_t port, uint32_t now,
                      RecoveryTiming timing) {
    if (port == 0 || port > locked_.size() || timing.off_ms == 0 ||
        timing.startup_ms == 0 || timing.discovery_ms == 0 ||
        timing.max_attempts == 0)
      return RecoveryStart::invalid;
    if (control_)
      return RecoveryStart::busy;
    if (!control.supports(port))
      return RecoveryStart::unsupported;
    if (locked_[port - 1])
      return RecoveryStart::locked;

    control_ = &control;
    port_ = port;
    timing_ = timing;
    attempts_ = 0;
    control_->inhibit_commands(port_);
    begin_attempt(now);
    return RecoveryStart::started;
  }

  void tick(uint32_t now) {
    if (!control_ || !check_health())
      return;
    const uint32_t elapsed = now - phase_started_;
    switch (phase_) {
      case Phase::off:
        if (elapsed >= timing_.off_ms) {
          if (!control_->set_power(port_, true)) {
            stop(RecoveryEvent::control_failed);
            return;
          }
          phase_ = Phase::startup;
          phase_started_ = now;
        }
        break;
      case Phase::startup:
        if (elapsed >= timing_.startup_ms) {
          if (!control_->isolate_data(port_, false) ||
              !control_->begin_discovery(port_)) {
            stop(RecoveryEvent::control_failed);
            return;
          }
          phase_ = Phase::discovery;
          phase_started_ = now;
        }
        break;
      case Phase::discovery:
        if (elapsed >= timing_.discovery_ms) {
          retry(now);
          return;
        }
        switch (control_->poll_discovery(port_)) {
          case Discovery::pending:
            break;
          case Discovery::recovered:
            // Output arming belongs to the product policy, never this sequence.
            control_->report(port_, attempts_, RecoveryEvent::recovered);
            control_ = nullptr;
            break;
          case Discovery::failed:
            retry(now);
            break;
        }
        break;
    }
  }

  bool busy() const { return control_ != nullptr; }

  // Explicit operator action after the underlying fault is investigated.
  bool clear_lock(uint8_t port) {
    if (busy() || port == 0 || port > locked_.size())
      return false;
    locked_[port - 1] = false;
    return true;
  }

 private:
  enum class Phase { off, startup, discovery };

  bool check_health() {
    switch (control_->health(port_)) {
      case PortHealth::ready:
        return true;
      case PortHealth::main_unavailable:
        stop(RecoveryEvent::unavailable);
        return false;
      case PortHealth::fault:
        stop(RecoveryEvent::fault);
        return false;
      case PortHealth::unknown:
        stop(RecoveryEvent::control_failed);
        return false;
    }
    return false;
  }

  void begin_attempt(uint32_t now) {
    if (!check_health())
      return;
    ++attempts_;
    control_->report(port_, attempts_, RecoveryEvent::attempt);
    if (!control_->isolate_data(port_, true) || !control_->set_power(port_, false)) {
      stop(RecoveryEvent::control_failed);
      return;
    }
    phase_ = Phase::off;
    phase_started_ = now;
  }

  void retry(uint32_t now) {
    if (attempts_ >= timing_.max_attempts) {
      stop(RecoveryEvent::exhausted);
      return;
    }
    control_->cancel_discovery(port_);
    begin_attempt(now);
  }

  void stop(RecoveryEvent event) {
    locked_[port_ - 1] = true;
    control_->cancel_discovery(port_);
    const bool isolated = control_->isolate_data(port_, true);
    const bool powered_off = control_->set_power(port_, false);
    control_->report(port_, attempts_, event);
    if ((!isolated || !powered_off) && event != RecoveryEvent::control_failed)
      control_->report(port_, attempts_, RecoveryEvent::control_failed);
    control_ = nullptr;
  }

  PortRecoveryControl *control_{nullptr};
  RecoveryTiming timing_{};
  std::array<bool, 8> locked_{};
  Phase phase_{Phase::off};
  uint32_t phase_started_{0};
  uint8_t port_{0};
  uint8_t attempts_{0};
};

}  // namespace trailnexus
