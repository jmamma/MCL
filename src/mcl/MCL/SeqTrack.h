/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQTRACK_H__
#define SEQTRACK_H__

#include "MCLMemory.h"
//#include "MidiActivePeering.h"
#include "MidiUartParent.h"
#include "platform.h"
#include "Global.h"
#include "SeqDefines.h"

#define EMPTY_TRACK_TYPE 0

#define SEQ_MUTE_ON 1
#define SEQ_MUTE_OFF 0

#define NUM_LOCKS 8

#define TRIG_FALSE 0
#define TRIG_TRUE 1
#define TRIG_ONESHOT 3

#define UART1_PORT 1

// Sequencer editing constants
#define DIR_LEFT 0
#define DIR_RIGHT 1
#define DIR_REVERSE 2

class SeqTrack_270 {};

class SlideData {
public:
  int16_t err;
  int8_t inc;
  int16_t dx;
  int16_t dy;
  int16_t x0;
  int8_t y0;
  int8_t y1;
  bool steep;
  int16_t x1;
  uint8_t yflip;
  ALWAYS_INLINE() void init() {
    dy = 0;
    dx = 0;
  }
};

// ephemeral data

class SeqTrackBase {
public:
  uint8_t active;

  uint8_t port = UART1_PORT;
  MidiUartClass *uart = &MidiUart;
  MidiUartClass *uart2 = &MidiUart2;
  uint8_t mute_state = SEQ_MUTE_OFF;

  bool record_mutes;

  uint8_t length;
  uint8_t speed;
  uint8_t track_number;

  uint8_t step_count;
  uint8_t mod12_counter;

  volatile uint8_t count_down;
  bool    cache_loaded;
  bool    load_sound;

  SeqTrackBase() { active = EMPTY_TRACK_TYPE; record_mutes = false; }

  ALWAYS_INLINE() void step_count_inc() {
    if (step_count == length - 1) {
      step_count = 0;

    } else {
      step_count++;
    }
  }

  ALWAYS_INLINE() void seq() {
    uint8_t timing_mid = get_timing_mid();
    mod12_counter++;
    if (count_down) {
      count_down--;
      if (count_down == 0) {
        reset();
        mod12_counter = 0;
      }
    }
    if (mod12_counter == timing_mid) {
      count_down = 0;
      mod12_counter = 0;
      step_count_inc();
    }
  }


  ALWAYS_INLINE() void reset() {
    mod12_counter = -1;
    step_count = 0;
    count_down = 0;
    cache_loaded = true; //Default should assume cache is loaded. Override in transition_load where required.
    load_sound = 0;
  }

  void toggle_mute() { mute_state = !mute_state; }

  uint8_t get_timing_mid(uint8_t speed_);
  uint8_t get_timing_mid() { return get_timing_mid(speed); }

  FORCED_INLINE() uint8_t get_timing_mid_inline() {
    uint8_t timing_mid;
    switch (speed) {
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


  void get_speed_multiplier(uint8_t speed_, uint8_t &n, uint8_t &d);

  float get_speed_multiplier(uint8_t speed_);

  float get_speed_multiplier() { return get_speed_multiplier(speed); }

  uint8_t get_quantized_step() {
    uint8_t u = 0;
    return get_quantized_step(u);
  }
  uint8_t get_quantized_step(uint8_t &utiming, uint8_t quant = 255);
};

class SeqTrack : public SeqTrackBase {

public:
  // Conditional counters
  uint8_t iterations_5;
  uint8_t iterations_6;
  uint8_t iterations_7;
  uint8_t iterations_8;

  uint16_t cur_event_idx;

  uint8_t ignore_step;

  SeqTrack() { reset(); }

  ALWAYS_INLINE() void reset() {
    cur_event_idx = 0;
    iterations_5 = 1;
    iterations_6 = 1;
    iterations_7 = 1;
    iterations_8 = 1;
    SeqTrackBase::reset();
    ignore_step = 255;
  }

  ALWAYS_INLINE() void seq() {
    uint8_t timing_mid = get_timing_mid();
    mod12_counter++;
    if (count_down) {
      count_down--;
      if (count_down == 0) {
        reset();
        mod12_counter = 0;
      }
    }
    if (mod12_counter == timing_mid) {
      count_down = 0;
      mod12_counter = 0;
      step_count_inc();
    }
  }

  ALWAYS_INLINE() void step_count_inc() {
    if (step_count == length - 1) {
      step_count = 0;
      cur_event_idx = 0;

      iterations_5++;
      iterations_6++;
      iterations_7++;
      iterations_8++;

      if (iterations_5 > 5) {
        iterations_5 = 1;
      }
      if (iterations_6 > 6) {
        iterations_6 = 1;
      }
      if (iterations_7 > 7) {
        iterations_7 = 1;
      }
      if (iterations_8 > 8) {
        iterations_8 = 1;
      }
    } else {
      step_count++;
    }
  }
  bool conditional(uint8_t condition);
};

class SeqSlideTrack : public SeqTrack {
public:
  SlideData locks_slide_data[NUM_LOCKS];
  uint8_t locks_slide_next_lock_val[NUM_LOCKS];
  uint8_t locks_slide_next_lock_step[NUM_LOCKS];
  uint8_t locks_slides_recalc = 255;
  uint16_t locks_slides_idx = 0;

  const uint8_t number_midi_cc = 6 * 2 + 4;
  const uint8_t midi_cc_array_size = 6 * 2 + 4;

  ALWAYS_INLINE() void reset() {
    for (uint8_t n = 0; n < NUM_LOCKS; n++) {
      locks_slide_data[n].init();
    }
    SeqTrack::reset();
  }


  void prepare_slide(uint8_t lock_idx, int16_t x0, int16_t x1, int8_t y0,
                     int8_t y1);
  void send_slides(volatile uint8_t *locks_params, uint8_t channel = 0);

};

#endif /* SEQTRACK_H__ */
