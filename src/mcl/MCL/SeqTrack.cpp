#include "SeqTrack.h"
#include "ArpSeqTrack.h"
#include "LFOSeqTrack.h"
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

  locks_slide_data[c].x0 = x0;
  locks_slide_data[c].x1 = x1;
  locks_slide_data[c].y0 = y0;
  locks_slide_data[c].y1 = y1;
  locks_slide_data[c].accum = 0;

  // Calculate delta in 8.8 fixed-point format
  int16_t steps = x1 - x0;
  if (steps > 0) {
    int16_t y_diff = y1 - y0;
    // delta = (y_diff << 8) / steps
    // To avoid overflow with 16-bit math, we use: delta = (y_diff * 256) / steps
    int32_t delta_calc = ((int32_t)y_diff << 8) / steps;
    locks_slide_data[c].delta = (uint16_t)delta_calc;
  } else {
    locks_slide_data[c].delta = 0;
  }
}

void SeqSlideTrack::send_slides(volatile uint8_t *locks_params, uint8_t channel) {
  on_slide_dispatch_begin(channel);
  for (uint8_t c = 0; c < NUM_LOCKS; c++) {
    if (locks_params[c] == 0) continue;
    if (locks_slide_data[c].x0 >= locks_slide_data[c].x1) continue;

    // Calculate current value using fixed-point (8.8 format)
    // accum is in 8.8 format, so we add 128 (0.5 in 8.8) for rounding, then shift
    int16_t val_calc = locks_slide_data[c].y0 + ((int16_t)(locks_slide_data[c].accum + 128) >> 8);
    uint8_t val = (uint8_t)val_calc;

    // Advance the slide
    locks_slide_data[c].accum += locks_slide_data[c].delta;
    locks_slide_data[c].x0++;

    // Check if we've reached the end
    if (locks_slide_data[c].x0 >= locks_slide_data[c].x1) {
      val = locks_slide_data[c].y1;  // Ensure we hit target exactly
      locks_slide_data[c].init();    // Mark as finished
    }

    uint8_t param = locks_params[c] - 1;
    dispatch_slide_value(param, val, channel);
  }
  on_slide_dispatch_end();
}

void SeqSlideTrack::on_slide_dispatch_begin(uint8_t) {}

void SeqSlideTrack::dispatch_slide_value(uint8_t param, uint8_t val,
                                         uint8_t channel) {
  if (param == PARAM_PB) {
    uart->sendPitchBend(channel, val << 7);
    return;
  }
  if (param == PARAM_CHP) {
    uart->sendChannelPressure(channel, val);
    return;
  }
  uart->sendCC(channel, param, val);
}

void SeqSlideTrack::on_slide_dispatch_end() {}

void SeqTrack::load_arp_data(ArpSeqTrack &arp_track,
                             const ArpSeqData &stored_data,
                             bool use_stored_data) {
  if (use_stored_data) {
    arp_track.load_data(stored_data);
    return;
  }
  ArpSeqData empty;
  empty.init();
  arp_track.load_data(empty);
}

void SeqTrack::load_lfo_data(LFOSeqTrack &lfo_track,
                             const SeqLFOData &stored_data,
                             bool use_stored_data) {
  if (use_stored_data) {
    lfo_track.load_data(stored_data);
    return;
  }
  SeqLFOData empty;
  empty.init();
  lfo_track.load_data(empty);
}

void SeqTrack::load_mod_data(SeqTrack *seq_track, SeqTrackModData &mod_data,
                             bool grid_x_tracks, bool use_stored_mod_data) {
  if (seq_track == nullptr) {
    return;
  }
  uint8_t tracknumber = seq_track->track_number;
  if (grid_x_tracks) {
    if (tracknumber >= NUM_GRID_X_LFO_TRACKS) {
      return;
    }
    load_arp_data(mcl_seq.md_arp_tracks[tracknumber], mod_data.arp,
                  use_stored_mod_data);
    load_lfo_data(mcl_seq.grid_x_lfo_tracks[tracknumber], mod_data.lfo,
                  use_stored_mod_data);
    return;
  }
#ifdef EXT_TRACKS
  if (tracknumber < NUM_GRID_Y_LFO_TRACKS) {
    load_arp_data(mcl_seq.ext_arp_tracks[tracknumber], mod_data.arp,
                  use_stored_mod_data);
    load_lfo_data(mcl_seq.grid_y_lfo_tracks[tracknumber], mod_data.lfo,
                  use_stored_mod_data);
  }
#endif
}

void SeqTrack::store_mod_data(SeqTrackModData &mod_data, bool grid_x_tracks,
                              uint8_t tracknumber) {
  if (grid_x_tracks) {
    if (tracknumber < NUM_GRID_X_LFO_TRACKS) {
      mcl_seq.md_arp_tracks[tracknumber].store_data(&mod_data.arp);
      mcl_seq.grid_x_lfo_tracks[tracknumber].store_data(&mod_data.lfo);
    } else {
      mod_data.arp.init();
      mod_data.lfo.init();
    }
    return;
  }
#ifdef EXT_TRACKS
  if (tracknumber < NUM_GRID_Y_LFO_TRACKS) {
    mcl_seq.ext_arp_tracks[tracknumber].store_data(&mod_data.arp);
    mcl_seq.grid_y_lfo_tracks[tracknumber].store_data(&mod_data.lfo);
  } else
#endif
  {
    mod_data.arp.init();
    mod_data.lfo.init();
  }
}

uint8_t SeqTrack::get_quantized_step(uint8_t &utiming, uint8_t quant) {
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

bool SeqTrackCond::conditional(uint8_t condition) {
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

uint8_t SeqTrack::get_timing_mid(uint8_t speed_) {
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
