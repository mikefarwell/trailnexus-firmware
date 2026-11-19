#include "../components/trailnexus_core/can_hub_power_role.h"
#include <cassert>
#include <initializer_list>

using namespace trailnexus::can;

int main() {
  for (uint8_t jacks : {3, 5}) {
    for (uint8_t j = 0; j < jacks; ++j) {
      HubPowerRoles hub(jacks);
      const auto upstream = static_cast<uint8_t>(1u << j);
      const auto outputs = static_cast<uint8_t>(((1u << jacks) - 1u) & ~upstream);
      assert(hub.evaluate({true, true, 0, false, false, 0}).status ==
             HubPowerStatus::waiting);
      auto d = hub.evaluate({true, true, upstream, false, false, 0});
      assert(d.status == HubPowerStatus::verifying && d.upstream_input == upstream &&
             !d.downstream_outputs);
      d = hub.evaluate({true, false, upstream, false, true, outputs});
      assert(d.status == HubPowerStatus::ready && d.upstream_input == upstream &&
             d.downstream_outputs == outputs);
      d = hub.evaluate({true, false, static_cast<uint8_t>(upstream | outputs),
                        false, true, outputs});
      assert(d.upstream_input == upstream && d.downstream_outputs == outputs);
      d = hub.evaluate({true, false, outputs, false, false, 0});
      assert(d.status == HubPowerStatus::locked && !d.upstream_input && !d.downstream_outputs);
      assert(!hub.reset(false));
      assert(hub.reset(true));
    }
  }

  HubPowerRoles hub(3);
  assert(hub.evaluate({true, true, 3, false, false, 0}).status == HubPowerStatus::locked);
  assert(hub.reset(true));
  assert(hub.evaluate({true, false, 1, false, false, 0}).status == HubPowerStatus::locked);
  assert(hub.reset(true));
  hub.evaluate({true, true, 1, false, false, 0});
  assert(hub.evaluate({true, false, 1, false, true, 1}).status == HubPowerStatus::locked);
  assert(hub.reset(true));
  hub.evaluate({true, true, 1, false, false, 0});
  assert(hub.evaluate({true, false, 1, true, true, 2}).status == HubPowerStatus::locked);
  assert(hub.reset(true));
  assert(hub.evaluate({false, true, 1, false, true, 2}).status == HubPowerStatus::locked);
  assert(hub.reset(true));
  assert(hub.evaluate({true, true, 8, false, false, 0}).status == HubPowerStatus::locked);
}
