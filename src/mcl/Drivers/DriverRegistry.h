#pragma once

#include <stdint.h>

class MidiDevice;

namespace DriverRegistry {

struct DriverList {
  MidiDevice **items;
  uint8_t count;
};

DriverList md_drivers();
DriverList elektron_drivers();
DriverList generic_drivers();

} // namespace DriverRegistry
