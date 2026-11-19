#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace trailnexus::can {

using Identity = std::array<uint8_t, 16>;
constexpr uint32_t query_base = 0x1A100000;
constexpr uint32_t reply_base = 0x1A100004;
constexpr uint8_t protocol_version = 1;

enum class Presence : uint8_t { zero, one, exact };

inline std::array<uint8_t, 8> query_fragment(const Identity &prefix, uint8_t bits,
                                              uint16_t token, uint8_t fragment) {
  std::array<uint8_t, 8> data{protocol_version, bits, uint8_t(token >> 8), uint8_t(token)};
  if (fragment < 4)
    for (std::size_t i = 0; i < 4; ++i)
      data[4 + i] = prefix[4 * fragment + i];
  return data;
}

inline std::optional<Presence> decode_reply(uint32_t id, const uint8_t *data,
                                             std::size_t size, uint16_t token) {
  if (id < reply_base || id > reply_base + 2 || !data || size != 3 ||
      data[0] != protocol_version || data[1] != uint8_t(token >> 8) ||
      data[2] != uint8_t(token))
    return std::nullopt;
  return static_cast<Presence>(id - reply_base);
}

class DiscoveryResponder {
 public:
  std::optional<Presence> ingest(const Identity &identity, uint32_t id,
                                  const uint8_t *data, std::size_t size,
                                  uint32_t now, uint32_t timeout_ms) {
    if (id < query_base || id >= query_base + 4)
      return std::nullopt;
    const auto fragment = id - query_base;
    if (!data || size != 8 || data[0] != protocol_version || data[1] > 128 ||
        timeout_ms == 0) {
      next_ = 0;
      return std::nullopt;
    }
    const uint8_t bits = data[1];
    const uint16_t token = uint16_t(data[2] << 8) | data[3];
    if (fragment == 0) {
      bits_ = bits;
      token_ = token;
      started_ = now;
      next_ = 0;
    }
    if (fragment != next_ || token != token_ || bits != bits_ ||
        now - started_ >= timeout_ms) {
      next_ = 0;
      return std::nullopt;
    }
    for (std::size_t i = 0; i < 4; ++i)
      prefix_[4 * fragment + i] = data[4 + i];
    if (++next_ != 4)
      return std::nullopt;
    next_ = 0;

    const auto byte = bits / 8;
    for (int i = 0; i < byte; ++i)
      if (prefix_[i] != identity[i])
        return std::nullopt;
    if (bits == 128)
      return Presence::exact;
    const uint8_t mask = uint8_t(0xFFu << (8 - bits % 8));
    if ((prefix_[byte] & mask) != (identity[byte] & mask))
      return std::nullopt;
    return (identity[byte] & (0x80u >> (bits % 8))) ? Presence::one : Presence::zero;
  }

 private:
  Identity prefix_{};
  uint32_t started_{0};
  uint16_t token_{0};
  uint8_t bits_{0};
  uint8_t next_{0};
};

}  // namespace trailnexus::can
