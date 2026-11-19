#pragma once

#include "can_discovery.h"
#include <vector>

namespace trailnexus::can {

struct Query {
  Identity prefix{};
  uint8_t bits{0};
  uint16_t token{0};
};

enum class ScanStatus { idle, scanning, complete, incomplete, limit, transport_error };

// One instance per chain. Drive all methods from the same task. The transport
// owns CAN health, ordered transmission of the four fragments, and deadlines.
class Enumerator {
 public:
  bool start(uint32_t window_ms, uint16_t query_limit, uint16_t node_limit) {
    if (status_ == ScanStatus::scanning || !window_ms || !query_limit || !node_limit)
      return false;
    window_ms_ = window_ms;
    query_limit_ = query_limit;
    node_limit_ = node_limit;
    queries_ = 0;
    waiting_ = false;
    pending_.clear();
    pending_.push_back({});
    identities_.clear();
    staged_.clear();
    status_ = ScanStatus::scanning;
    return true;
  }

  std::optional<Query> next_query(uint32_t now) {
    if (status_ != ScanStatus::scanning)
      return std::nullopt;
    if (waiting_) {
      if (now - sent_at_ < window_ms_)
        return std::nullopt;
      waiting_ = false;
      if (replies_ == 0 && current_.bits != 0) {
        finish(ScanStatus::incomplete);
        return std::nullopt;
      }
      if (current_.bits == 128) {
        if (staged_.size() >= node_limit_) {
          finish(ScanStatus::limit);
          return std::nullopt;
        }
        staged_.push_back(current_.prefix);
      } else {
        if (replies_ & bit(Presence::one))
          push_child(true);
        if (replies_ & bit(Presence::zero))
          push_child(false);
      }
    }

    if (pending_.empty()) {
      finish(ScanStatus::complete);
      return std::nullopt;
    }
    if (queries_ == query_limit_) {
      finish(ScanStatus::limit);
      return std::nullopt;
    }

    current_ = pending_.back();
    pending_.pop_back();
    current_.token = ++token_;
    ++queries_;
    replies_ = 0;
    sent_at_ = now;
    waiting_ = true;
    return current_;
  }

  void receive(uint32_t id, const uint8_t *data, std::size_t size, uint32_t now) {
    if (!waiting_ || now - sent_at_ >= window_ms_)
      return;
    const auto reply = decode_reply(id, data, size, current_.token);
    if (reply && ((*reply == Presence::exact) == (current_.bits == 128)))
      replies_ |= bit(*reply);
  }

  void transport_failed() {
    if (status_ == ScanStatus::scanning)
      finish(ScanStatus::transport_error);
  }

  ScanStatus status() const { return status_; }
  // Results are available only after an entirely successful scan.
  const std::vector<Identity> &identities() const { return identities_; }

 private:
  static uint8_t bit(Presence presence) { return 1u << static_cast<uint8_t>(presence); }

  void push_child(bool one) {
    Query child = current_;
    if (one)
      child.prefix[child.bits / 8] |= 0x80u >> (child.bits % 8);
    ++child.bits;
    pending_.push_back(child);
  }

  void finish(ScanStatus status) {
    status_ = status;
    waiting_ = false;
    if (status == ScanStatus::complete)
      identities_.swap(staged_);
    staged_.clear();
  }

  ScanStatus status_{ScanStatus::idle};
  Query current_{};
  std::vector<Query> pending_;
  std::vector<Identity> identities_;
  std::vector<Identity> staged_;
  uint32_t sent_at_{0};
  uint32_t window_ms_{0};
  uint16_t token_{0};
  uint16_t queries_{0};
  uint16_t query_limit_{0};
  uint16_t node_limit_{0};
  uint8_t replies_{0};
  bool waiting_{false};
};

}  // namespace trailnexus::can
