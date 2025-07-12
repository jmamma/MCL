#include "MCLGif.h"
#include "global.h"

uint8_t *MCLGif::get_next_frame() {
  uint8_t *bmp = get_frame(cur_frame);
  if (clock_diff(last_frame_clock, g_clock_ms) < 80 || loop_count == loops) {
    return bmp;
  }
  last_frame_clock = g_clock_ms;
  cur_frame += inc;

  if (cur_frame < 0) {
    cur_frame = 0;
    inc = 1;
    loop_count++;
  }

  if (cur_frame == num_of_frames) {
    if (dir == DIR_FWD) {
      loop_count++;
      cur_frame = 0;
    }
    if (dir == DIR_FWDBACK) {
      cur_frame -= 2;
      inc = -1;
    }
  }
  return bmp;
}
