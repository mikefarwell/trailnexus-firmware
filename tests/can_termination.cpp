#include "../components/trailnexus_core/can_termination.h"
#include <cassert>
#include <initializer_list>

using namespace trailnexus::can;

int main() {
  for (uint8_t ports : {2, 4, 8}) {
    CoreTermination policy;
    const auto valid = static_cast<uint8_t>((1u << ports) - 1u);
    auto d = policy.update(ports, 0, valid, UINT32_MAX, 10, false);
    assert(d.mode == TerminationMode::settling && !d.connect_core_terminator);
    d = policy.update(ports, 0, valid, 9, 10, false);
    assert(d.mode == TerminationMode::idle && d.connect_core_terminator == false && !d.can_operate);
    const auto last = static_cast<uint8_t>(1u << (ports - 1));
    d = policy.update(ports, last, valid, 10, 10, false);
    assert(d.mode == TerminationMode::settling && !d.can_operate);
    d = policy.update(ports, last, valid, 20, 10, false);
    assert(d.mode == TerminationMode::one_run && d.connect_core_terminator == true && !d.can_operate);
    assert(policy.update(ports, last, valid, 21, 10, true).can_operate);
    const auto two = static_cast<uint8_t>(last | 1u);
    assert(!policy.update(ports, two, valid, 30, 10, true).can_operate);
    d = policy.update(ports, two, valid, 40, 10, true);
    assert(d.mode == TerminationMode::two_runs && d.connect_core_terminator == false && !d.can_operate);
    assert(policy.update(ports, two, valid, 41, 10, false).can_operate);
    assert(policy.update(ports, two, 0, 42, 10, false).mode == TerminationMode::sense_fault);
    assert(policy.update(ports, two, valid, 43, 10, false).mode == TerminationMode::settling);
  }
  CoreTermination policy;
  policy.update(4, 7, 15, 10, 10, true);
  const auto d = policy.update(4, 7, 15, 20, 10, true);
  assert(d.mode == TerminationMode::unsupported && !d.connect_core_terminator && !d.can_operate);
  assert(policy.update(4, 16, 15, 30, 10, false).mode == TerminationMode::sense_fault);
  assert(policy.update(9, 0, 255, 40, 10, false).mode == TerminationMode::sense_fault);
  static_assert(module_termination(false) && !module_termination(true));
}
