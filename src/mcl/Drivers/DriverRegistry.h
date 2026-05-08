#pragma once

#include <stdint.h>

class MidiDevice;

namespace DriverRegistry {

struct DriverList {
  MidiDevice **items;
  uint8_t count;
};

namespace detail {

constexpr uint8_t MD_DRIVER_COUNT = 1;
constexpr uint8_t ELEKTRON_DRIVER_COUNT = 3;
constexpr uint8_t GENERIC_DRIVER_COUNT = 1;

extern MidiDevice *md_slot_driver_list[MD_DRIVER_COUNT];
extern MidiDevice *elektron_slot_driver_list[ELEKTRON_DRIVER_COUNT];
extern MidiDevice *generic_driver_list[GENERIC_DRIVER_COUNT];

} // namespace detail

#ifdef PLATFORM_TBD
DriverList md_drivers();
DriverList elektron_drivers();
DriverList generic_drivers();
#else
inline DriverList md_drivers() {
  return {detail::md_slot_driver_list, detail::MD_DRIVER_COUNT};
}

inline DriverList elektron_drivers() {
  return {detail::elektron_slot_driver_list, detail::ELEKTRON_DRIVER_COUNT};
}

inline DriverList generic_drivers() {
  return {detail::generic_driver_list, detail::GENERIC_DRIVER_COUNT};
}
#endif

} // namespace DriverRegistry
