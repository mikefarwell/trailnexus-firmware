#include "../components/trailnexus_core/port_recovery.h"
#include <cassert>
#include <vector>

using namespace trailnexus;

struct Fake : PortRecoveryControl {
  bool supported = true;
  bool fail_on = false;
  PortHealth current = PortHealth::ready;
  Discovery discovery = Discovery::pending;
  std::vector<char> actions;
  std::vector<RecoveryEvent> events;

  bool supports(uint8_t) const override { return supported; }
  PortHealth health(uint8_t port) override {
    assert(port == 6);
    return current;
  }
  void inhibit_commands(uint8_t) override { actions.push_back('I'); }
  bool isolate_data(uint8_t, bool isolate) override {
    actions.push_back(isolate ? 'D' : 'd');
    return true;
  }
  bool set_power(uint8_t, bool enabled) override {
    actions.push_back(enabled ? 'P' : 'p');
    return !enabled || !fail_on;
  }
  bool begin_discovery(uint8_t) override {
    actions.push_back('S');
    return true;
  }
  Discovery poll_discovery(uint8_t) override { return discovery; }
  void cancel_discovery(uint8_t) override { actions.push_back('C'); }
  void report(uint8_t port, uint8_t attempt, RecoveryEvent event) override {
    assert(port == 6 && attempt <= 2);
    events.push_back(event);
  }
};

int main() {
  const RecoveryTiming timing{10, 20, 30, 2};
  PortRecovery recovery;
  Fake hw;
  assert(recovery.start(hw, 9, 0, timing) == RecoveryStart::invalid);
  hw.supported = false;
  assert(recovery.start(hw, 6, 0, timing) == RecoveryStart::unsupported);
  assert(hw.actions.empty());
  hw.supported = true;

  assert(recovery.start(hw, 6, UINT32_MAX - 4, timing) == RecoveryStart::started);
  assert(recovery.start(hw, 7, 0, timing) == RecoveryStart::busy);
  recovery.tick(4);
  assert((hw.actions == std::vector<char>{'I', 'D', 'p'}));
  recovery.tick(5);
  recovery.tick(25);
  assert((hw.actions == std::vector<char>{'I', 'D', 'p', 'P', 'd', 'S'}));
  hw.discovery = Discovery::recovered;
  recovery.tick(26);
  assert(!recovery.busy() && hw.events.back() == RecoveryEvent::recovered);

  hw.discovery = Discovery::pending;
  assert(recovery.start(hw, 6, 100, timing) == RecoveryStart::started);
  for (auto now : {110u, 130u, 160u, 170u, 190u, 220u})
    recovery.tick(now);
  assert(!recovery.busy() && hw.events.back() == RecoveryEvent::exhausted);
  assert(hw.actions.back() == 'p');
  assert(recovery.start(hw, 6, 230, timing) == RecoveryStart::locked);
  assert(recovery.clear_lock(6));

  assert(recovery.start(hw, 6, 240, timing) == RecoveryStart::started);
  hw.current = PortHealth::main_unavailable;
  recovery.tick(250);
  assert(!recovery.busy() && hw.events.back() == RecoveryEvent::unavailable);
  assert(hw.actions.back() == 'p');
  assert(recovery.clear_lock(6));

  hw.current = PortHealth::ready;
  hw.fail_on = true;
  assert(recovery.start(hw, 6, 260, timing) == RecoveryStart::started);
  recovery.tick(270);
  assert(!recovery.busy() && hw.events.back() == RecoveryEvent::control_failed);
  assert(hw.actions.back() == 'p');
  assert(recovery.clear_lock(6));

  hw.fail_on = false;
  hw.current = PortHealth::fault;
  assert(recovery.start(hw, 6, 280, timing) == RecoveryStart::started);
  assert(!recovery.busy() && hw.events.back() == RecoveryEvent::fault);
  assert(hw.actions.back() == 'p');
}
