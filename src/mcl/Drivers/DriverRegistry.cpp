#include "DriverRegistry.h"

#include "A4/A4.h"
#include "Generic/GenericMidiDevice.h"
#include "MD/MD.h"
#include "MNM/MNM.h"

namespace {

MidiDevice *md_slot_driver_list[] = {
    &MD,
};

MidiDevice *elektron_slot_driver_list[] = {
    &MNM,
    &Analog4,
    &generic_midi_device,
};

MidiDevice *generic_driver_list[] = {
    &generic_midi_device,
};

template <size_t N>
DriverRegistry::DriverList make_list(MidiDevice *(&items)[N]) {
  static_assert(N <= UINT8_MAX, "DriverList count exceeds uint8_t");
  return {items, static_cast<uint8_t>(N)};
}

} // namespace

namespace DriverRegistry {

DriverList md_drivers() {
  return make_list(md_slot_driver_list);
}

DriverList elektron_drivers() {
  return make_list(elektron_slot_driver_list);
}

DriverList generic_drivers() {
  return make_list(generic_driver_list);
}

} // namespace DriverRegistry
