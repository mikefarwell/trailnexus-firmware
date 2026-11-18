#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <utility>

namespace trailnexus {

using BoardId = std::array<uint8_t, 16>;

struct Descriptor {
  BoardId id;
  uint16_t product;
  uint16_t revision;
  uint8_t inputs;
  uint8_t outputs;
  uint16_t eeprom_size;
};

// Development format v1. The transport reads this prefix before registration.
inline std::optional<Descriptor> decode_descriptor(const uint8_t *bytes, std::size_t size) {
  constexpr std::size_t payload_size = 29;
  if (bytes == nullptr || size < payload_size + 2 ||
      bytes[0] != 'T' || bytes[1] != 'N' || bytes[2] != 'B' || bytes[3] != 'D' ||
      bytes[4] != 1)
    return std::nullopt;

  const auto read_u16 = [bytes](std::size_t offset) {
    return static_cast<uint16_t>(bytes[offset] | (bytes[offset + 1] << 8));
  };

  uint16_t crc = 0xffff;
  for (std::size_t offset = 0; offset < payload_size; ++offset) {
    crc ^= static_cast<uint16_t>(bytes[offset] << 8);
    for (int bit = 0; bit < 8; ++bit)
      crc = static_cast<uint16_t>((crc << 1) ^ ((crc & 0x8000) ? 0x1021 : 0));
  }

  Descriptor descriptor{{}, read_u16(21), read_u16(23), bytes[25], bytes[26],
                        read_u16(27)};
  for (std::size_t index = 0; index < descriptor.id.size(); ++index)
    descriptor.id[index] = bytes[5 + index];

  BoardId erased;
  erased.fill(0xff);
  if (read_u16(payload_size) != crc || descriptor.id == BoardId{} ||
      descriptor.id == erased || descriptor.product == 0 ||
      descriptor.eeprom_size < payload_size + 2)
    return std::nullopt;

  return descriptor;
}

enum class Status { ok, missing, duplicate, out_of_range, transport_error };

// Route tokens are ephemeral. Implementations must reject stale routes and
// verify the expected board identity before accessing a physical endpoint.
class Transport {
 public:
  virtual ~Transport() = default;
  virtual Status read_input(uint32_t route, const BoardId &id, uint8_t channel,
                            int32_t &value) = 0;
  virtual Status write_output(uint32_t route, const BoardId &id, uint8_t channel,
                              int32_t value) = 0;
  virtual Status read_eeprom(uint32_t route, const BoardId &id, uint16_t offset,
                             uint8_t *data, std::size_t size) = 0;
};

class BoardRegistry {
 public:
  struct Entry {
    Descriptor descriptor;
    uint32_t route;
    bool duplicate{false};
  };

  using Inventory = std::map<BoardId, Entry>;

  static bool observe(Inventory &inventory, uint32_t route,
                      const uint8_t *descriptor, std::size_t size) {
    const auto board = decode_descriptor(descriptor, size);
    if (!board)
      return false;
    auto [entry, inserted] = inventory.emplace(board->id, Entry{*board, route});
    if (!inserted)
      entry->second.duplicate = true;
    return true;
  }

  // Publish a complete scan. Clear with an empty inventory on transport loss.
  // The transport must outlive this binding. Use from the owning task only.
  void replace_inventory(Transport &transport, Inventory inventory) {
    transport_ = &transport;
    inventory_ = std::move(inventory);
  }

  const Inventory &inventory() const { return inventory_; }

  Status read_input(const BoardId &id, uint8_t channel, int32_t &value) {
    const Entry *entry;
    if (const auto status = resolve(id, entry); status != Status::ok)
      return status;
    if (channel >= entry->descriptor.inputs)
      return Status::out_of_range;
    return transport_->read_input(entry->route, id, channel, value);
  }

  Status write_output(const BoardId &id, uint8_t channel, int32_t value) {
    const Entry *entry;
    if (const auto status = resolve(id, entry); status != Status::ok)
      return status;
    if (channel >= entry->descriptor.outputs)
      return Status::out_of_range;
    return transport_->write_output(entry->route, id, channel, value);
  }

  Status read_eeprom(const BoardId &id, uint16_t offset, uint8_t *data,
                     std::size_t size) {
    const Entry *entry;
    if (const auto status = resolve(id, entry); status != Status::ok)
      return status;
    const std::size_t limit = entry->descriptor.eeprom_size;
    if (offset > limit || size > limit - offset || (size != 0 && data == nullptr))
      return Status::out_of_range;
    if (size == 0)
      return Status::ok;
    return transport_->read_eeprom(entry->route, id, offset, data, size);
  }

 private:
  Status resolve(const BoardId &id, const Entry *&entry) const {
    const auto found = inventory_.find(id);
    if (found == inventory_.end())
      return Status::missing;
    entry = &found->second;
    return entry->duplicate ? Status::duplicate : Status::ok;
  }

  Transport *transport_{nullptr};
  Inventory inventory_;
};

}  // namespace trailnexus
