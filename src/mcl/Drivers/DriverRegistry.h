#pragma once

#include <stddef.h>

class MidiDevice;

namespace DriverRegistry {

struct DriverList {
  MidiDevice **items;
  size_t count;
};

DriverList md_drivers();
DriverList elektron_drivers();
DriverList generic_drivers();

} // namespace DriverRegistry
