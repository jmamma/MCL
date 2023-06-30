#include "PerfEncoder.h"
#include "MCLMemory.h"
#include "MD.h"

#define DIV_1_127 (1.00f / 127.0f)

void PerfEncoder::send_params(uint8_t cur_, uint8_t scene) {
    for (uint8_t n = 0; n < NUM_PERF_PARAMS; n++) {
       if (perf_data.params[n].dest == 0) { continue; }
       uint8_t val = 0;
       uint8_t dest = perf_data.params[n].dest - 1;
       uint8_t param = perf_data.params[n].param;
       uint8_t min = perf_data.params[n].scenes[active_scene_a];
       uint8_t max = perf_data.params[n].scenes[active_scene_b];
       int8_t range = max - min;
       int16_t q = cur_ * range;
       if (scene == 255) {
          val = ((int16_t) q / (int16_t) 128) + min;
       }
       else {
          val = perf_data.params[n].scenes[scene];
       }

       if (dest >= NUM_MD_TRACKS + 4) {
          uint8_t channel = dest - NUM_MD_TRACKS;
          MidiUart2.sendCC(channel, param, val);
        }
       else if (dest >= NUM_MD_TRACKS ) {
          MD.sendFXParam(param, val, MD_FX_ECHO + dest - NUM_MD_TRACKS);
          setLed2();
       }
       else {
          MD.setTrackParam_inline(dest, param, val);
       }
    }
}

int PerfEncoder::update(encoder_t *enc) {
  MCLEncoder::update(enc);
  //Update all params
  if (hasChanged()) {
    send_params(cur);
  }
  return cur;
}
