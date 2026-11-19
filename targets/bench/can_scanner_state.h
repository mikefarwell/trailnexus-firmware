#pragma once

#include "can_enumerator.h"

inline trailnexus::can::Enumerator &bench_can_scan() {
  static trailnexus::can::Enumerator scanner;
  return scanner;
}
