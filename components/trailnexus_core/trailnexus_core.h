#pragma once

#include "esphome/core/component.h"
#include "board_registry.h"

namespace esphome::trailnexus_core {

class Core : public Component {
 public:
  trailnexus::BoardRegistry &boards() { return boards_; }

 private:
  trailnexus::BoardRegistry boards_;
};

}  // namespace esphome::trailnexus_core
