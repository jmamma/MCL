#include "MCLEncoder.h"
#include "helpers.h"

int MCLEncoder::update(encoder_t *enc) {
  int inc = 0;
  if (enc) {
    inc = update_rotations(enc);
    inc = enc->button ? inc * fast_speed : inc;
  }
  cur = limit_value(cur, inc, min, max);
  return cur;
}

int MCLExpEncoder::update(encoder_t *enc) {
  int inc = update_rotations(enc);

  if ((!enc->button) && inc != 0) {
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
    inc = inc;
  }
  cur = limit_value(cur, inc, min, max);
  return cur;
}

int8_t consume_centered_encoder_delta(EncoderParent *enc) {
  int8_t diff = (int8_t)(enc->cur - enc->old);
  enc->cur = 64 + diff;
  enc->old = 64;
  return diff;
}


int MCLRelativeEncoder::update(encoder_t *enc) {
  old = 0;
  cur = 0;
  if (enc) {
    cur = update_rotations(enc);
    if (enc->button) {
      cur *= fast_speed;
    }
  }
  return cur;
}
