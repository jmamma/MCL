/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQTRACK_H__
#define SEQTRACK_H__

#include "MCLMemory.h"
#include "MidiActivePeering.h"
#include "MidiUartParent.h"
#include "WProgram.h"

#define EMPTY_TRACK_TYPE 0

#define SEQ_MUTE_ON 1
#define SEQ_MUTE_OFF 0

#define SEQ_SPEED_1X 0
#define SEQ_SPEED_2X 1
#define SEQ_SPEED_3_4X 2
#define SEQ_SPEED_3_2X 3
#define SEQ_SPEED_1_2X 4
#define SEQ_SPEED_1_4X 5
#define SEQ_SPEED_1_8X 6

#define MASK_PATTERN 0
#define MASK_LOCK 1
#define MASK_SLIDE 2
#define MASK_MUTE 3
#define MASK_LOCKS_ON_STEP 4

#define NUM_LOCKS 8

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
  MidiUartParent *uart = &MidiUart;
  uint8_t mute_state = SEQ_MUTE_OFF;

  uint8_t length;
  uint8_t speed;
  uint8_t track_number;

  uint8_t step_count;
  uint8_t mod12_counter;

  SeqTrackBase() { active = EMPTY_TRACK_TYPE; }

  ALWAYS_INLINE() void step_count_inc() {
    if (step_count == length - 1) {
      step_count = 0;

    } else {
      step_count++;
    }
  }

  ALWAYS_INLINE() void reset() {
    mod12_counter = 0;
    step_count = 0;
  }

  uint8_t get_timing_mid(uint8_t speed_) {
    uint8_t timing_mid;
    switch (speed_) {
    default:
    case SEQ_SPEED_1X:
      timing_mid = 12;
      break;
    case SEQ_SPEED_2X:
      timing_mid = 6;
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

  float get_speed_multiplier() { return get_speed_multiplier(speed); }

  float get_speed_multiplier(uint8_t speed_) {
    float multi;
    switch (speed_) {
    default:
    case SEQ_SPEED_1X:
      multi = 1;
      break;
    case SEQ_SPEED_2X:
      multi = 0.5;
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
};

class SeqTrack : public SeqTrackBase {

public:
  // Conditional counters
  uint8_t iterations_5;
  uint8_t iterations_6;
  uint8_t iterations_7;
  uint8_t iterations_8;

  uint8_t count_down;

  uint16_t cur_event_idx;

  SeqTrack() { reset(); }

  ALWAYS_INLINE() void reset() {
    count_down = 0;
    cur_event_idx = 0;
    iterations_5 = 1;
    iterations_6 = 1;
    iterations_7 = 1;
    iterations_8 = 1;
    SeqTrackBase::reset();
  }

  ALWAYS_INLINE() void seq() {
    uint8_t timing_mid = get_timing_mid();
    mod12_counter++;
    if (mod12_counter == timing_mid) {
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
};

class SeqSlideTrack : public SeqTrack {
public:
  SlideData locks_slide_data[NUM_LOCKS];
  uint8_t locks_slide_next_lock_val[NUM_LOCKS];
  uint8_t locks_slide_next_lock_step[NUM_LOCKS];
  uint8_t locks_slides_recalc = 255;
  uint16_t locks_slides_idx = 0;

  void prepare_slide(uint8_t lock_idx, int16_t x0, int16_t x1, int8_t y0,
                     int8_t y1);
  void send_slides(volatile uint8_t *locks_params, uint8_t channel = 0);
};

#endif /* SEQTRACK_H__ */
