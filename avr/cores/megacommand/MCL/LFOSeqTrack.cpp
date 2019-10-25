#include "LFOSeqTrack.h"
#include "LFO.h"
#include "MCL.h"

void LFOSeqTrack::seq() {

  if ((MidiClock.mod12_counter == 0) && (mode != LFO_MODE_FREE) &&
      IS_BIT_SET64(pattern_mask, step_count)) {
    sample_count = 0;
  }
  if (enable) {
    for (uint8_t i = 0; i < NUM_OF_LFO_PARAMS; i++) {

         // MD CC LFO
      if (params[i].dest < NUM_MD_TRACKS) {
        MD.setTrackParam_inline(params[i].dest, params[i].param,
                                wav_table[sample_count]);
      }
      // MD FX LFO
      else if (params[i].dest != 255) {
        MD.sendFXParam(params[i].param, wav_table[sample_count], params[i].dest);
      }
    }
  }

  sample_count += speed;
  if (sample_count > LFO_LENGTH) {
    // Free running LFO should reset, oneshot should hold at last value.
    if (mode == LFO_MODE_ONE) {
      sample_count = LFO_LENGTH;
    } else {
      sample_count = 0;
    }
  }

  if (MidiClock.mod12_counter == 11) {
    if (step_count == length - 1) {
      step_count = 0;
    } else {
      step_count++;
    }
  }
}
