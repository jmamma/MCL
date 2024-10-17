#include "MCL_impl.h"

void PerfSeqTrack::seq() {
  uint8_t timing_mid = get_timing_mid();
  mod12_counter++;
  if (mod12_counter == timing_mid) {
    mod12_counter = 0;
    step_count_inc();
  }
  if (count_down) {
    count_down--;
    if (count_down == 0) {
      if (load_sound) {
        for (uint8_t n = 0; n < 4; n++) {
          if (perf_locks[n] != 255) {
            mixer_page.encoders[n]->cur = perf_locks[n];
            mixer_page.encoders[n]->old = perf_locks[n] + 1;
          }
          perf_locks[n] = 255;
        }
      }
      reset();
      mod12_counter = 0;
    }
  }
}
