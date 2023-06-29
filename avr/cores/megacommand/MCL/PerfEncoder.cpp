#include "PerfEncoder.h"
#include "MCLMemory.h"
#include "MD.h"

#define DIV_1_127 (1.00f / 127.0f)

void PerfEncoder::send_params() {
    for (uint8_t n = 0; n < NUM_PERF_PARAMS; n++) {
       if (perf_data.params[n].dest == 0) { continue; }
       MidiUartClass *uart = &MidiUart2;

       uint8_t dest = perf_data.params[n].dest - 1;
       uint8_t param = perf_data.params[n].param;
       uint8_t min = perf_data.params[n].min;
       uint8_t max = perf_data.params[n].max;
       int8_t range = max - min;
       int16_t q = cur * range;
       uint8_t val = ((int16_t) q / (int16_t) 128) + min;
       if (dest > NUM_MD_TRACKS + 4) {
          uint8_t channel = dest - NUM_MD_TRACKS;
          uart->sendCC(channel, param, val);
        }
       else if (dest > NUM_MD_TRACKS ) {
          MD.sendFXParam(param, val, MD_FX_ECHO + dest - NUM_MD_TRACKS);
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
    send_params();
  }
  return cur;
}
