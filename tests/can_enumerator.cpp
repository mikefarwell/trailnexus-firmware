#include "../components/trailnexus_core/can_enumerator.h"
#include <cassert>

using namespace trailnexus::can;

int main() {
  Enumerator core;
  const Identity a{0x80, 1}, b{0x80, 2}, c{0x01, 3};
  assert(core.start(5, 512, 8));
  assert(!core.start(5, 512, 8));
  uint32_t now = UINT32_MAX - 2;
  while (auto q = core.next_query(now)) {
    const uint8_t reply[]{1, uint8_t(q->token >> 8), uint8_t(q->token)};
    for (const auto &id : {a, b, c}) {
      DiscoveryResponder node;
      for (uint8_t f = 0; f < 4; ++f) {
        auto data = query_fragment(q->prefix, q->bits, q->token, f);
        auto presence = node.ingest(id, query_base + f, data.data(), data.size(), now, 4);
        if (presence) {
          const auto reply_id = reply_base + static_cast<uint8_t>(*presence);
          core.receive(reply_id, reply, 3, now);
          core.receive(reply_id, reply, 3, now + 1);
        }
      }
    }
    assert(core.identities().empty());
    assert(!core.next_query(now + 4));
    now += 5;
  }
  assert(core.status() == ScanStatus::complete);
  assert((core.identities() == std::vector<Identity>{c, a, b}));
  assert(core.start(5, 2, 8));
  auto q = core.next_query(100);
  assert(q);
  const uint8_t response[]{1, uint8_t(q->token >> 8), uint8_t(q->token)};
  core.receive(reply_base, response, 3, 100);
  assert(core.next_query(105));
  core.receive(reply_base, response, 3, 105);
  assert(!core.next_query(110));
  assert(core.status() == ScanStatus::incomplete && core.identities().empty());

  assert(core.start(5, 1, 8));
  q = core.next_query(200);
  assert(q);
  const uint8_t fresh[]{1, uint8_t(q->token >> 8), uint8_t(q->token)};
  core.receive(reply_base + 1, fresh, 3, 200);
  assert(!core.next_query(205));
  assert(core.status() == ScanStatus::limit && core.identities().empty());

  assert(core.start(5, 5, 8));
  assert(core.next_query(300));
  core.transport_failed();
  assert(!core.next_query(305));
  assert(core.status() == ScanStatus::transport_error && core.identities().empty());

  assert(core.start(5, 5, 8));
  assert(core.next_query(400));
  assert(!core.next_query(405));
  assert(core.status() == ScanStatus::complete && core.identities().empty());
}
