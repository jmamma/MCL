#include "MCL_impl.h"


int MCLEncoder::update(encoder_t *enc) {
  int inc = update_rotations(enc);
  inc = inc + (pressmode ? 0 : (fastmode ? 4 * enc->button : enc->button));
  cur = limit_value(cur, inc, min, max);

  return cur;
}

int MCLExpEncoder::update(encoder_t *enc) {
  int inc = update_rotations(enc);

  if (!pressmode && fastmode && inc != 0) {
    int8_t r = 0;
    while (cur >>= 1) {
      r++;
    }
    if (inc > 0) { r += 1; }
    if (inc < 0) { r -= 1; }
    r = max(0,r);
    cur = 1 << r;
    inc = 0;
  }
  else {
    inc = inc + enc->button;
  }
  cur = limit_value(cur, inc, min, max);
  return cur;
}
