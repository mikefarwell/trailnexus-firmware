#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "board_registry.h"
#include "port_recovery.h"

namespace esphome::trailnexus_core {

class Core : public Component {
 public:
  trailnexus::BoardRegistry &boards() { return boards_; }

  trailnexus::PortRecovery &port_recovery() { return port_recovery_; }
  void loop() override { port_recovery_.tick(millis()); }

 private:
  trailnexus::BoardRegistry boards_;
  trailnexus::PortRecovery port_recovery_;
};

}  // namespace esphome::trailnexus_core
