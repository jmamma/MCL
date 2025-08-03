#include "SeqTrack.h"
#include "MCLSeq.h"
#include "memory.h"
#include "MCLSysConfig.h"

#define _swap_int16_t(a, b)                                                    \
  {                                                                            \
    int16_t t = a;                                                             \
    a = b;                                                                     \
    b = t;                                                                     \
  }
#define _swap_int8_t(a, b)                                                     \
  {                                                                            \
    int8_t t = a;                                                              \
    a = b;                                                                     \
    b = t;                                                                     \
  }

void SeqSlideTrack::prepare_slide(uint8_t lock_idx, int16_t x0, int16_t x1, int8_t y0, int8_t y1) {
  uint8_t c = lock_idx;
  locks_slide_data[c].steep = abs(y1 - y0) < abs(x1 - x0);
  locks_slide_data[c].yflip = 255;
  if (locks_slide_data[c].steep) {
    /* Disable as this use case will not exist.
    if (x0 > x1) {
          _swap_int16_t(x0, x1);
         _swap_int16_t(y0, y1);
    }
    */
  } else {
    if (y0 > y1) {
      _swap_int8_t(y0, y1);
      _swap_int16_t(x0, x1);
      locks_slide_data[c].yflip = y0;
    }
  }
  locks_slide_data[c].dx = x1 - x0;
  locks_slide_data[c].dy = y1 - y0;
  locks_slide_data[c].inc = 1;

  if (locks_slide_data[c].steep) {
    if (locks_slide_data[c].dy < 0) {
      locks_slide_data[c].inc = -1;
      locks_slide_data[c].dy *= -1;
    }
    locks_slide_data[c].dy *= 2;
    locks_slide_data[c].err = locks_slide_data[c].dy - locks_slide_data[c].dx;
    locks_slide_data[c].dx *= 2;
  } else {
    if (locks_slide_data[c].dx < 0) {
      locks_slide_data[c].inc = -1;
      locks_slide_data[c].dx *= -1;
    }

    locks_slide_data[c].dx *= 2;
    locks_slide_data[c].err = locks_slide_data[c].dx - locks_slide_data[c].dy;
    locks_slide_data[c].dy *= 2;
  }
  locks_slide_data[c].y0 = y0;
  locks_slide_data[c].x0 = x0;
  locks_slide_data[c].x1 = x1;
  locks_slide_data[c].y1 = y1;
/*
  DEBUG_DUMP(step);
  DEBUG_DUMP(locks_slide_data[c].x0);
  DEBUG_DUMP(locks_slide_data[c].y0);
  DEBUG_DUMP(x1);
  DEBUG_DUMP(y1);
  DEBUG_DUMP(locks_slide_data[c].dx);
  DEBUG_DUMP(locks_slide_data[c].dy);
  DEBUG_DUMP(locks_slide_data[c].steep);
  DEBUG_DUMP(locks_slide_data[c].inc);
  DEBUG_DUMP(locks_slide_data[c].yflip);
*/
}

void SeqSlideTrack::send_slides(volatile uint8_t *locks_params, uint8_t channel) {
  uint8_t ccs[midi_cc_array_size];
  bool send_ccs = false;
  bool is_midi_model = (MD.kit.models[track_number] & 0xF0) == MID_01_MODEL;
  if (is_midi_model) {
    memset(ccs, 255, sizeof(ccs));
  }
  for (uint8_t c = 0; c < NUM_LOCKS; c++) {
    if ((locks_params[c] > 0) && (locks_slide_data[c].dy > 0)) {

      uint8_t val;
      val = locks_slide_data[c].y0;
      if (locks_slide_data[c].steep) {
        if (locks_slide_data[c].err > 0) {
          locks_slide_data[c].y0 += locks_slide_data[c].inc;
          locks_slide_data[c].err -= locks_slide_data[c].dx;
        }
        locks_slide_data[c].err += locks_slide_data[c].dy;
        locks_slide_data[c].x0++;
        if (locks_slide_data[c].x0 >= locks_slide_data[c].x1) {
          locks_slide_data[c].init();
          break;
        }
      } else {
        uint16_t x0_old = locks_slide_data[c].x0;
        while (locks_slide_data[c].x0 == x0_old) {
          if (locks_slide_data[c].err > 0) {
            locks_slide_data[c].x0 += locks_slide_data[c].inc;
            locks_slide_data[c].err -= locks_slide_data[c].dy;
          }
          locks_slide_data[c].err += locks_slide_data[c].dx;
          locks_slide_data[c].y0++;
          if (locks_slide_data[c].y0 >= locks_slide_data[c].y1) {
            locks_slide_data[c].init();
            break;
          }
        }
        if (locks_slide_data[c].yflip != 255) {
          val = locks_slide_data[c].y1 - val + locks_slide_data[c].yflip;
        }
      }
      uint8_t param = locks_params[c] - 1;
      switch (active) {
      case MD_TRACK_TYPE:
        if (is_midi_model) {
          uint8_t p = param;
          send_ccs |= (p > 4 && p < 8) | (p > 8) && (p & 1) | (p == 20);
          mcl_seq.md_tracks[track_number].process_note_locks(p, val, ccs, true);
        }
        else {
          MD.setTrackParam_inline(track_number, param, val);
        }
        break;
      default:
        if (param == PARAM_PB) {
          uart->sendPitchBend(channel, val << 7);
          break;
        }
        if (param == PARAM_CHP) {
          uart->sendChannelPressure(channel, val);
          break;
        }
        uart->sendCC(channel, param, val);
        break;
      }
    }
  }
  if (is_midi_model) {
    mcl_seq.md_tracks[track_number].send_notes_ccs(ccs, send_ccs);
  }
}

uint8_t SeqTrackBase::get_quantized_step(uint8_t &utiming, uint8_t quant) {
  if (quant == 255) { quant = mcl_cfg.rec_quant; }

  uint8_t timing_mid = get_timing_mid();

  int8_t mod12 = mod12_counter - 1;


  uint8_t step = step_count;
/*
  if ((step == 0) && (mod12 < 0)) {
    mod12 += timing_mid;
    step = length - 1;
  }
*/
  utiming = mod12 + timing_mid;

  if (quant) {
    if (mod12 > timing_mid / 2) {
      step++;
      if (step == length) {
        step = 0;
      }
    }
    utiming = timing_mid;
  }
  return step;
}

bool SeqTrack::conditional(uint8_t condition) {
  bool send_note = false;
  uint8_t random_byte = 0;
  if (condition >= 9) { random_byte = get_random_byte(); }

  switch (condition) {
  case 0:
  case 1:
    send_note = true;
    break;
  case 2:
    if (!IS_BIT_SET(iterations_8, 0)) {
      send_note = true;
    }
    break;
  case 3:
    if ((iterations_6 == 3) || (iterations_6 == 6)) {
      send_note = true;
    }
    break;
  case 6:
    if (iterations_6 == 6) {
      send_note = true;
    }
    break;
  case 4:
    if ((iterations_8 == 4) || (iterations_8 == 8)) {
      send_note = true;
    }
    break;
  case 8:
    if (iterations_8 == 8) {
      send_note = true;
    }
    break;
  case 5:
    if (iterations_5 == 5) {
      send_note = true;
    }
    break;
  case 7:
    if (iterations_7 == 7) {
      send_note = true;
    }
    break;
  case 9:
    if (random_byte <= 26) {
      send_note = true;
    }
    break;
  case 10:
    if (random_byte <= 64) {
      send_note = true;
    }
    break;
  case 11:
    if (random_byte <= 128) {
      send_note = true;
    }
    break;
  case 12:
    if (random_byte <= 192) {
      send_note = true;
    }
    break;
  case 13:
    if (random_byte <= 230) {
      send_note = true;
    }
    break;
  }
  return send_note;
}

uint8_t SeqTrackBase::get_timing_mid(uint8_t speed_) {
  uint8_t timing_mid;
  switch (speed_) {
  default:
  case SEQ_SPEED_1X:
    timing_mid = 12;
    break;
  case SEQ_SPEED_2X:
    timing_mid = 6;
    break;
  case SEQ_SPEED_4X:
    timing_mid = 3;
    break;
  case SEQ_SPEED_3_4X:
    timing_mid = 16; // 12 * (4.0/3.0);
    break;
  case SEQ_SPEED_3_2X:
    timing_mid = 8; // 12 * (2.0/3.0);
    break;
  case SEQ_SPEED_1_2X:
    timing_mid = 24;
    break;
  case SEQ_SPEED_1_4X:
    timing_mid = 48;
    break;
  case SEQ_SPEED_1_8X:
    timing_mid = 96;
    break;
  }
  return timing_mid;
}

void SeqTrackBase::get_speed_multiplier(uint8_t speed_, uint8_t &n, uint8_t &d) {
  n = 1;
  d = 1;
  switch (speed_) {
  default:
  case SEQ_SPEED_1X:
    // n = 1;
    // d = 1;
    break;
  case SEQ_SPEED_2X:
    // n = 1;
    d = 2;
    break;
  case SEQ_SPEED_4X:
    // n = 1;
    d = 4;
    break;
  case SEQ_SPEED_3_4X:
    n = 4;
    d = 3;
    break;
  case SEQ_SPEED_3_2X:
    n = 2;
    d = 3;
    break;
  case SEQ_SPEED_1_2X:
    n = 2;
    // d = 1;
    break;
  case SEQ_SPEED_1_4X:
    n = 4;
    // d = 1;
    break;
  case SEQ_SPEED_1_8X:
    n = 8;
    // d = 1;
    break;
  }
}

float SeqTrackBase::get_speed_multiplier(uint8_t speed_) {
  float multi;
  switch (speed_) {
  default:
  case SEQ_SPEED_1X:
    multi = 1;
    break;
  case SEQ_SPEED_2X:
    multi = 0.5;
    break;
  case SEQ_SPEED_4X:
    multi = 0.25;
    break;
  case SEQ_SPEED_3_4X:
    multi = (4.0 / 3.0);
    break;
  case SEQ_SPEED_3_2X:
    multi = (2.0 / 3.0);
    break;
  case SEQ_SPEED_1_2X:
    multi = 2.0;
    break;
  case SEQ_SPEED_1_4X:
    multi = 4.0;
    break;
  case SEQ_SPEED_1_8X:
    multi = 8.0;
    break;
  }
  return multi;
}
