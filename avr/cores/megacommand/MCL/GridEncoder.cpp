#include "GridEncoder.h"
#include "MCL.h"

int GridEncoder::update(encoder_t *enc) {
  uint8_t amount = abs(enc->normal);
  int inc = 0;

  while (amount > 0) {
    if (enc->normal > 0) {
      rot_counter_up += 1;
      if (rot_counter_up > rot_res) {
        rot_counter_up = 0;
        inc += 1;
      }
      rot_counter_down = 0;
    }
    if (enc->normal < 0) {
      rot_counter_down += 1;
      if (rot_counter_down > rot_res) {
        rot_counter_down = 0;
        inc -= 1;
      }

      rot_counter_up = 0;
    }
    amount--;
  }
  inc = inc + (pressmode ? 0 : (fastmode ? 5 * enc->button : enc->button));
  cur = limit_value(cur, inc, min, max);

  return cur;
}

void GridEncoder::displayAt(int i) {}
