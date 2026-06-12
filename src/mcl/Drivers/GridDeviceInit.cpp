#include "GridDeviceInit.h"

#if defined(__AVR__)
#include "GridTrack.h"
#include "Sequencer/MCLSeq.h"

void init_ext_track_grid_devices(MidiDevice &device, DeviceIdx device_idx,
                                 uint8_t track_type) {
  GridDeviceTrack gdt;

  for (uint8_t i = 0; i < NUM_EXT_TRACKS; i++) {
    gdt.init(track_type, GROUP_DEV, static_cast<uint8_t>(device_idx),
             &(mcl_seq.ext_tracks[i]));
    device.add_track_to_grid(GridIdx::Y, i, &gdt);
  }
}
#endif
