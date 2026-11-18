#include "../components/trailnexus_core/board_registry.h"
#include <cassert>

using namespace trailnexus;

class FakeTransport : public Transport {
 public:
  uint32_t last_route = 0;
  Status read_input(uint32_t route, const BoardId &, uint8_t, int32_t &value) override {
    last_route = route;
    value = 1;
    return Status::ok;
  }
  Status write_output(uint32_t route, const BoardId &, uint8_t, int32_t) override {
    last_route = route;
    return Status::ok;
  }
  Status read_eeprom(uint32_t route, const BoardId &, uint16_t, uint8_t *data,
                     std::size_t) override {
    last_route = route;
    *data = 42;
    return Status::ok;
  }
};

int main() {
  // CRC-16/CCITT-FALSE computed independently over the first 29 bytes.
  std::array<uint8_t, 31> bytes{
      'T', 'N', 'B', 'D', 1, 42, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 1, 0, 2, 0, 1, 1, 128, 0, 89, 216};
  BoardId id{42};
  BoardRegistry registry;
  FakeTransport transport;
  BoardRegistry::Inventory scan;
  int32_t value = 0;
  uint8_t data = 0;
  assert(registry.read_input(id, 0, value) == Status::missing);
  assert(BoardRegistry::observe(scan, 1, bytes.data(), bytes.size()));
  assert(scan.at(id).descriptor.revision == 2);
  registry.replace_inventory(transport, std::move(scan));
  assert(registry.read_input(id, 0, value) == Status::ok && value == 1);
  assert(transport.last_route == 1);

  scan.clear();
  assert(BoardRegistry::observe(scan, 603, bytes.data(), bytes.size()));
  registry.replace_inventory(transport, std::move(scan));
  assert(registry.write_output(id, 0, 1) == Status::ok && transport.last_route == 603);
  assert(registry.write_output(id, 1, 1) == Status::out_of_range);
  assert(registry.read_eeprom(id, 127, &data, 1) == Status::ok && data == 42);
  assert(registry.read_eeprom(id, 127, &data, 2) == Status::out_of_range);

  scan.clear();
  assert(BoardRegistry::observe(scan, 1, bytes.data(), bytes.size()));
  assert(BoardRegistry::observe(scan, 603, bytes.data(), bytes.size()));
  registry.replace_inventory(transport, std::move(scan));
  assert(registry.write_output(id, 0, 1) == Status::duplicate);
  registry.replace_inventory(transport, {});
  assert(registry.write_output(id, 0, 1) == Status::missing);

  bytes[25] ^= 1;
  assert(!decode_descriptor(bytes.data(), bytes.size()));
  assert(!decode_descriptor(bytes.data(), 30));
}
