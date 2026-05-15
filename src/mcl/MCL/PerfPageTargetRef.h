#pragma once

#include "DeviceParamResolver.h"
#include "NoteInterface.h"
#include "SeqPages.h"
#include <inttypes.h>

class PerfPageTargetRef {
public:
  static uint8_t target_count() {
    return DeviceParamResolver::perf_target_count();
  }

  static DevicePerfTarget target(uint8_t dest) {
    return DeviceParamResolver::perf(dest);
  }

  static uint8_t active_editor_dest() {
    return DeviceParamResolver::primary_perf_editor_dest(
        seq_primary_track_index());
  }

  static bool begin_editor(uint8_t dest, uint8_t *params, uint8_t count) {
    return target(dest).begin_param_editor(params, count);
  }

  static void end_editor() { DeviceParamResolver::end_perf_param_editor(); }

  static void set_rec_mode(uint8_t mode) {
    DeviceParamResolver::set_perf_rec_mode(mode);
  }

  static uint8_t pressed_scene() {
    return note_interface.get_first_trig_note();
  }
};
