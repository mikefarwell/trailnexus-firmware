#include "../components/trailnexus_core/can_discovery.h"
#include <cassert>

using namespace trailnexus::can;

int main() {
  Identity a{0x80, 1}, b{0x80, 2}, other{0x00, 3};
  DiscoveryResponder responder;
  const auto query = [&](const Identity &node, const Identity &prefix, uint8_t bits) {
    std::optional<Presence> result;
    for (uint8_t f = 0; f < 4; ++f) {
      const auto data = query_fragment(prefix, bits, 0x1234, f);
      result = responder.ingest(node, query_base + f, data.data(), data.size(), f, 10);
      if (f < 3)
        assert(!result);
    }
    return result;
  };
  assert(query(a, {}, 0) == Presence::one);
  assert(query(other, {}, 0) == Presence::zero);
  assert(query(other, a, 1) == std::nullopt);
  assert(query(a, a, 14) == Presence::zero);
  assert(query(b, a, 14) == Presence::one);
  assert(query(a, a, 128) == Presence::exact);
  assert(query(b, a, 128) == std::nullopt);
  const auto data = query_fragment(a, 128, 1, 0);
  assert(!responder.ingest(a, query_base, data.data(), data.size(), 0, 5));
  assert(!responder.ingest(a, query_base + 1, data.data(), data.size(), 6, 5));
  for (uint8_t f = 2; f < 4; ++f)
    assert(!responder.ingest(a, query_base + f, data.data(), data.size(), 7, 5));
  const uint8_t reply[]{1, 0x12, 0x34};
  assert(decode_reply(reply_base + 1, reply, 3, 0x1234) == Presence::one);
  assert(!decode_reply(reply_base + 1, reply, 3, 0x1235));
  assert(!decode_reply(reply_base + 3, reply, 3, 0x1234));
}
