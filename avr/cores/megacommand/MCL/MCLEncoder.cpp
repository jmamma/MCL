#include "MCLEncoder.h"

int MCLEncoder::update(encoder_t *enc) {
  int inc = enc->normal;
  // int inc = 4 + (pressmode ? 0 : (fastmode ? 5 * enc->button : enc->button));

  rot_counter += enc->normal;
  if (rot_counter > rot_res) {
    cur = limit_value(cur, inc, min, max);
    rot_counter = 0;
  } else if (rot_counter < 0) {
    cur = limit_value(cur, inc, min, max);
    rot_counter = rot_res;
  }

  return cur;
}
