#include "SeqTrack.h"
#include "SeqCondition.h"
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

static void load_arp_mod_data(ArpSeqTrack &arp, SeqTrack *seq_track,
                              SeqTrackModData &mod_data) NOINLINE();
static void load_arp_mod_data(ArpSeqTrack &arp, SeqTrack *seq_track,
                              SeqTrackModData &mod_data) {
  arp.load_data(mod_data.arp, mod_data.arp_phase(), seq_track->speed);
  if (seq_track->count_down) {
#if defined(__AVR__)
    arp.count_down =
        seq_track->count_down == 255 ? 255 : seq_track->count_down + 1;
#else
    arp.count_down = seq_track->count_down + 1;
#endif
  }
}

static void store_arp_mod_data(ArpSeqTrack &arp,
                               SeqTrackModData &mod_data) NOINLINE();
static void store_arp_mod_data(ArpSeqTrack &arp, SeqTrackModData &mod_data) {
  arp.store_data(&mod_data.arp);
  arp.store_phase_data(mod_data.arp_phase());
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

void SeqTrack::load_mod_data(SeqTrack *seq_track, SeqTrackModData &mod_data,
                             bool grid_x_tracks) {
  if (seq_track == nullptr) {
    return;
  }
  uint8_t tracknumber = seq_track->track_number;
  if (grid_x_tracks) {
    if (tracknumber >= NUM_GRID_X_LFO_TRACKS) {
      return;
    }
    load_arp_mod_data(mcl_seq.md_arp_tracks[tracknumber], seq_track,
                      mod_data);
    mcl_seq.grid_x_lfo_tracks[tracknumber].load_data(mod_data.lfo);
    return;
  }
#ifdef EXT_TRACKS
  if (tracknumber < NUM_GRID_Y_LFO_TRACKS) {
    load_arp_mod_data(mcl_seq.ext_arp_tracks[tracknumber], seq_track,
                      mod_data);
    mcl_seq.grid_y_lfo_tracks[tracknumber].load_data(mod_data.lfo);
  }
#endif
}

void SeqTrack::store_mod_data(SeqTrackModData &mod_data, bool grid_x_tracks,
                              uint8_t tracknumber) {
  if (grid_x_tracks) {
    if (tracknumber < NUM_GRID_X_LFO_TRACKS) {
      store_arp_mod_data(mcl_seq.md_arp_tracks[tracknumber], mod_data);
      mcl_seq.grid_x_lfo_tracks[tracknumber].store_data(&mod_data.lfo);
    } else {
      mod_data.arp.init();
      mod_data.arp_phase().init();
      mod_data.lfo.init();
    }
    return;
  }
#ifdef EXT_TRACKS
  if (tracknumber < NUM_GRID_Y_LFO_TRACKS) {
    store_arp_mod_data(mcl_seq.ext_arp_tracks[tracknumber], mod_data);
    mcl_seq.grid_y_lfo_tracks[tracknumber].store_data(&mod_data.lfo);
  } else
#endif
  {
    mod_data.arp.init();
    mod_data.arp_phase().init();
    mod_data.lfo.init();
  }
}

uint8_t SeqTrack::get_quantized_step(uint8_t &utiming, uint8_t quant) {
  if (quant == 255) { quant = mcl_cfg.rec_quant; }

  uint8_t ticks_per_step = get_ticks_per_step();

  int8_t mod12 = mod12_counter - 1;


  uint8_t step = step_count;
/*
  if ((step == 0) && (mod12 < 0)) {
    mod12 += ticks_per_step;
    step = length - 1;
  }
*/
  utiming = mod12 + ticks_per_step;

  if (quant) {
    if (mod12 > ticks_per_step / 2) {
      step++;
      if (step == length) {
        step = 0;
      }
    }
    utiming = ticks_per_step;
  }
  return step;
}

bool seq_cond_iter_decode(uint8_t cond, uint8_t &x, uint8_t &y) {
  if (cond < SEQ_COND_ITER_BASE || cond > SEQ_COND_ITER_MAX) return false;
  uint8_t offset = cond - SEQ_COND_ITER_BASE;
  for (uint8_t i = 2; i <= 8; i++) {
    if (offset < i) {
      y = i;
      x = offset + 1;
      return true;
    }
    offset -= i;
  }
  return false;
}

void SeqTrackCond::record_trig_result(bool fired) {
  seq_condition_set_prev_trig(conditional_flags, fired);
  seq_condition_record_neighbor(mcl_seq.neighbor_trig_mask, track_number,
                                fired);
}

bool SeqTrackCond::neighbor_fired() const {
  return seq_condition_neighbor_fired(mcl_seq.neighbor_trig_mask, track_number);
}

bool SeqTrackCond::conditional(uint8_t condition) {
  return conditional(condition, mcl_seq.fill_mask_for(DeviceIdx::Primary));
}

bool SeqTrackCond::conditional(uint8_t condition, uint16_t fill_mask) {
  return seq_condition_match(condition, iterations, conditional_flags,
                             track_number, fill_mask,
                             mcl_seq.neighbor_trig_mask);
}

void seq_condition_label(uint8_t condition, bool plock, bool marker,
                         char *out) {
  if (out == nullptr) {
    return;
  }

  char a = '-';
  char b = '-';
  char d = '-';

  if (condition <= SEQ_COND_NOT_NEI) {
    static const char labels[] PROGMEM =
        "---10%25%33%50%66%75%90%1SH1ST!1SFIL!FLPRE!PRNEI!NE";
    uint8_t idx = condition + condition + condition;
    a = (char)pgm_read_byte_near(labels + idx);
    b = (char)pgm_read_byte_near(labels + idx + 1);
    d = (char)pgm_read_byte_near(labels + idx + 2);
  } else {
    uint8_t x, y;
    if (seq_cond_iter_decode(condition, x, y)) {
      a = (char)('0' + x);
      b = ':';
      d = (char)('0' + y);
    }
  }

  out[0] = a;
  out[1] = b;
  out[2] = d;
  uint8_t i = 3;
  if (plock) {
    out[i++] = marker ? '+' : '^';
  }
  out[i] = '\0';
}

uint8_t SeqTrack::get_ticks_per_step(uint8_t speed_) {
  uint8_t ticks_per_step;
  switch (speed_) {
  default:
  case SEQ_SPEED_1X:
    ticks_per_step = 12;
    break;
  case SEQ_SPEED_2X:
    ticks_per_step = 6;
    break;
  case SEQ_SPEED_4X:
    ticks_per_step = 3;
    break;
  case SEQ_SPEED_3_4X:
    ticks_per_step = 16; // 12 * (4.0/3.0);
    break;
  case SEQ_SPEED_3_2X:
    ticks_per_step = 8; // 12 * (2.0/3.0);
    break;
  case SEQ_SPEED_1_2X:
    ticks_per_step = 24;
    break;
  case SEQ_SPEED_1_4X:
    ticks_per_step = 48;
    break;
  case SEQ_SPEED_1_8X:
    ticks_per_step = 96;
    break;
  }
  return ticks_per_step;
}

int16_t SeqTrack::microtiming_to_ticks(int8_t microtiming,
                                       uint16_t ticks_per_step) {
  if (ticks_per_step == 0 || microtiming == 0) {
    return 0;
  }
  int16_t numerator = (int16_t)microtiming * ticks_per_step;
  int16_t ticks = (numerator + (numerator >= 0 ? 64 : -64)) / 128;
  if (ticks < -(int16_t)ticks_per_step) {
    ticks = -(int16_t)ticks_per_step;
  } else if (ticks >= (int16_t)ticks_per_step) {
    ticks = (int16_t)ticks_per_step - 1;
  }
  return ticks;
}

int8_t SeqTrack::ticks_to_microtiming(int16_t ticks,
                                      uint16_t ticks_per_step) {
  if (ticks_per_step == 0 || ticks == 0) {
    return 0;
  }
  int16_t numerator = ticks * 128;
  uint16_t half = ticks_per_step >> 1;
  int16_t value = numerator >= 0
                      ? (int16_t)((uint16_t)(numerator + half) / ticks_per_step)
                      : (int16_t)-(((uint16_t)(-numerator) + half) /
                                   ticks_per_step);
  if (value < -128) {
    value = -128;
  } else if (value > 127) {
    value = 127;
  }
  return (int8_t)value;
}

uint16_t SeqTrack::microtiming_to_timing(int8_t microtiming,
                                         uint16_t ticks_per_step) {
  return ticks_per_step + microtiming_to_ticks(microtiming, ticks_per_step);
}

int8_t SeqTrack::timing_to_microtiming(uint16_t timing,
                                       uint16_t ticks_per_step) {
  return ticks_to_microtiming((int16_t)timing - (int16_t)ticks_per_step,
                              ticks_per_step);
}
