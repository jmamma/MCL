/* Copyright 2026, Justin Mammarella jmamma@gmail.com */

#include "TrackLoadFadeTarget.h"
#include "GridTrack.h"
#include "Grid/MidiDeviceGrid.h"
#include "TrackLoadFade.h"
#include "../Drivers/MD/MD.h"
#include "../Drivers/MD/MDParams.h"

namespace {

bool track_type_is_md(uint8_t track_type) {
  return track_type == MD_TRACK_TYPE || track_type == MDSPSX_TRACK_TYPE;
}

} // namespace

bool resolve_track_load_fade_target(GridSlot slot,
                                    GridDeviceTrack *gdt,
                                    const TrackLoadFadeData *fade,
                                    TrackLoadFadeTarget *target) {
  if (gdt == nullptr || fade == nullptr || target == nullptr) {
    return false;
  }
  if (gdt->device_idx >= NUM_DEVS) {
    return false;
  }
  if (!track_type_is_md(gdt->track_type)) {
    return false;
  }
  if (fade->target != TRACK_LOAD_FADE_TARGET_DEFAULT &&
      fade->target != MODEL_LEVEL) {
    return false;
  }
  target->track_type = gdt->track_type;
  target->device_idx = gdt->device_idx;
  target->track_number = slot & 0x0F;
  target->param = MODEL_LEVEL;
  return true;
}

bool read_track_load_fade_value(const TrackLoadFadeTarget &target,
                                uint8_t *value) {
  if (value == nullptr) {
    return false;
  }
  if (!track_type_is_md(target.track_type)) {
    return false;
  }
  *value = MD.kit.levels[target.track_number];
  return true;
}

void write_track_load_fade_value(const TrackLoadFadeTarget &target,
                                 uint8_t value,
                                 MidiUartClass *uart,
                                 MidiUartClass *uart2) {
  if (!track_type_is_md(target.track_type)) {
    return;
  }
  MidiUartClass *output = target.device_idx == 1 ? uart2 : uart;
  MD.setTrackParam(target.track_number, target.param, value, output, false);
}
