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
  return {items, N};
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
