#pragma once

#include "MidiDevice.h"

#if defined(__AVR__)
void init_ext_track_grid_devices(MidiDevice &device, DeviceIdx device_idx,
                                 uint8_t track_type);
#endif
