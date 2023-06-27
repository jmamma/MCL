#include "PerfEncoder.h"
#include "MCLMemory.h"
#include "MD.h"

#define DIV_1_127 (1.00f / 127.0f)

int PerfEncoder::update(encoder_t *enc) {
  MCLEncoder::update(enc);
  //Update all params
  if (hasChanged()) {
    for (uint8_t n = 0; n < NUM_PERF_PARAMS; n++) {
       if (perf_data.params[n].dest == 0) { continue; }
       MidiUartClass *uart = &MidiUart;
       if (perf_data.params[n].dest > 20) { uart = &MidiUart2; }

       uint8_t dest = perf_data.params[n].dest - 1;
       uint8_t param = perf_data.params[n].param;
       uint8_t min = perf_data.params[n].min;
       uint8_t max = perf_data.params[n].max;
       int8_t range = max - min;
       uint16_t q = cur * range;
       uint8_t val = ((float)q *  DIV_1_127) + min;

       if (dest < NUM_MD_TRACKS) {
           MD.setTrackParam_inline(dest, param, val, uart);
       }
       if (dest < NUM_MD_TRACKS + 4) {

       }
       else if (dest < NUM_MD_TRACKS + 4 + 16) {
          uint8_t channel = dest - NUM_MD_TRACKS;
          uart->sendCC(channel, param, val);
        }
       }
  }
  return cur;
}
