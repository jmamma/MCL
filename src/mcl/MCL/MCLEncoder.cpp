#include "MCLEncoder.h"
#include "helpers.h"

int MCLEncoder::update(encoder_t *enc) {
  int inc = 0;
  if (enc) {
    inc = update_rotations(enc);
    inc = fastmode ? (enc->button ? inc * 4 : inc) : inc;
  }
  cur = limit_value(cur, inc, min, max);
  return cur;
}

int MCLExpEncoder::update(encoder_t *enc) {
  int inc = update_rotations(enc);

  if (fastmode && inc != 0) {
    int8_t r = 0;
    while (cur >>= 1) {
      r++;
    }
    if (inc > 0) { r += 1; }
    if (inc < 0) { r -= 1; }
    r = MAX(0, r);
    cur = 1 << r;
    inc = 0;
  }
  else {
    inc = inc + enc->button;
  }
  cur = limit_value(cur, inc, min, max);
  return cur;
}

int MCLRelativeEncoder::update(encoder_t *enc) {
  changed = false;
  if (cur == old) {
    int inc = 0;
    if (enc) {
      inc = update_rotations(enc);
      inc = fastmode ? (enc->button ? inc * 4 : inc) : inc;
    }
    cur = inc;
  }
  if (cur != old) { changed = true; }
  old = cur;
  return cur;
}

bool MCLRelativeEncoder::hasChanged() {
    return changed;
}
