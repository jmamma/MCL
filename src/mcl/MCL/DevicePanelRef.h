#pragma once

#include <stdint.h>

#if defined(__AVR__)
#include "../Drivers/MD/MD.h"
#else
#include "../Drivers/MidiDevice.h"
#include "DeviceManager.h"
#endif

namespace DevicePanelRef {

inline void set_primary_key_repeat(uint8_t enabled) {
#if defined(__AVR__)
  MD.set_key_repeat(enabled);
#else
  device_manager.primary_device()->panel()->set_key_repeat(enabled);
#endif
}

} // namespace DevicePanelRef
