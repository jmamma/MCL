#include "PerfSeqTrack.h"
#include "CommonPages.h"

void PerfSeqTrack::seq(MidiUartClass *uart_, MidiUartClass *uart2_) {
  uint8_t ticks_per_step = get_ticks_per_step();
  mod12_counter++;
  if (mod12_counter == ticks_per_step) {
    mod12_counter = 0;
    step_count_inc();
  }
  if (count_down) {
    count_down--;
    if (count_down == 0) {
      if (load_sound) {
        for (uint8_t n = 0; n < 4; n++) {
          if (perf_locks[n] != 255) {
            perf_page.perf_encoders[n]->cur = perf_locks[n];
            perf_page.perf_encoders[n]->old = perf_locks[n];
            perf_page.send_perf_encoder(n, uart_, uart2_);
            //perf_page.perf_encoders[n]->resend = true;
          }
          perf_locks[n] = 255;
        }
        load_sound = 0;
      }
      reset();
      mod12_counter = 0;
    }
  }
}
