#include "DriverRegistry.h"

#include "A4/A4.h"
#include "Generic/GenericMidiDevice.h"
#include "MD/MD.h"
#include "MNM/MNM.h"

namespace DriverRegistry {
namespace detail {

MidiDevice *md_slot_driver_list[MD_DRIVER_COUNT] = {
    &MD,
};

MidiDevice *elektron_slot_driver_list[ELEKTRON_DRIVER_COUNT] = {
    &MNM,
    &Analog4,
    &generic_midi_device,
};

MidiDevice *generic_driver_list[GENERIC_DRIVER_COUNT] = {
    &generic_midi_device,
};

} // namespace detail
} // namespace DriverRegistry

#ifdef PLATFORM_TBD
namespace DriverRegistry {

DriverList md_drivers() {
  return {detail::md_slot_driver_list, detail::MD_DRIVER_COUNT};
}

DriverList elektron_drivers() {
  return {detail::elektron_slot_driver_list, detail::ELEKTRON_DRIVER_COUNT};
}

DriverList generic_drivers() {
  return {detail::generic_driver_list, detail::GENERIC_DRIVER_COUNT};
}

} // namespace DriverRegistry
#endif
