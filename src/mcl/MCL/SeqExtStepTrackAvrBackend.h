#pragma once

#include "../Drivers/MD/MD.h"
#include "MCLSeq.h"
#include "SeqExtStepTrackApi.h"
#include "SeqTrackUtil.h"
#include <stdint.h>

class SeqExtStepTrackAvrBackend {
public:
  static SeqExtStepTrackApi track(uint8_t i) {
    return SeqTrackUtil::get_ext_step_track(i);
  }

  static uint8_t track_count() {
    return SeqTrackUtil::track_count(false);
  }

  static void set_panel_rec_mode(uint8_t mode) {
    MD.set_rec_mode(mode);
  }
};
