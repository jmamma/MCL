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
  int16_t x0;      // Current step position
  int16_t x1;      // End step position
  uint16_t accum;  // Fixed-point accumulator (8.8 format)
  uint16_t delta;  // Fixed-point delta per step (8.8 format)
  int8_t y0;       // Start value
  int8_t y1;       // Target value
  ALWAYS_INLINE() void init() {
    x0 = 0;
    x1 = 0;
    accum = 0;
    delta = 0;
  }
};

// ephemeral data

class SeqTrack {
public:
  uint8_t active;
  struct PendingSpeedChange {
    uint8_t value : 7;
    uint8_t active : 1;
  };

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
  volatile bool    cache_loaded;
  bool    load_sound;
  PendingSpeedChange pending_speed_change{};

  SeqTrack() { active = EMPTY_TRACK_TYPE; record_mutes = false; }

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
    pending_speed_change.active = 0;
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

  static uint16_t get_speed_multiplier_int(uint8_t speed) {
  switch (speed) {
    case SEQ_SPEED_2X:   return 6;  // 0.5  * 12
    case SEQ_SPEED_4X:   return 3;  // 0.25 * 12
    case SEQ_SPEED_3_4X: return 16; // 1.33 * 12
    case SEQ_SPEED_3_2X: return 8;  // 0.66 * 12
    case SEQ_SPEED_1_2X: return 24; // 2.0  * 12
    case SEQ_SPEED_1_4X: return 48; // 4.0  * 12
    case SEQ_SPEED_1_8X: return 96; // 8.0  * 12
    default:
    case SEQ_SPEED_1X:   return 12; // 1.0  * 12
  }
  }

  void get_speed_multiplier(uint8_t speed_, uint8_t &n, uint8_t &d);

  float get_speed_multiplier(uint8_t speed_);

  float get_speed_multiplier() { return get_speed_multiplier(speed); }
  uint16_t get_speed_multiplier_int() { return get_speed_multiplier_int(speed); }

  uint8_t get_quantized_step() {
    uint8_t u = 0;
    return get_quantized_step(u);
  }
  uint8_t get_quantized_step(uint8_t &utiming, uint8_t quant = 255);

  ALWAYS_INLINE() void queue_speed_change(uint8_t new_speed) {
    pending_speed_change.value = new_speed & 0x7F;
    pending_speed_change.active = 1;
  }

  ALWAYS_INLINE() bool has_pending_speed_change() const {
    return pending_speed_change.active;
  }

  ALWAYS_INLINE() bool consume_pending_speed_change(uint8_t &new_speed) {
    if (!has_pending_speed_change()) {
      return false;
    }
    new_speed = pending_speed_change.value;
    pending_speed_change.active = 0;
    return true;
  }

  ALWAYS_INLINE() void clear_pending_speed_change() {
    pending_speed_change.active = 0;
  }

  ALWAYS_INLINE() bool request_speed_change(uint8_t new_speed) {
    if (count_down) {
      return false;
    }
    if (speed == new_speed && !has_pending_speed_change()) {
      return false;
    }
    queue_speed_change(new_speed);
    return true;
  }
};

class SeqTrackCond : public SeqTrack {

public:
  // Conditional counters
  uint8_t iterations_5;
  uint8_t iterations_6;
  uint8_t iterations_7;
  uint8_t iterations_8;

  uint16_t cur_event_idx;

  uint8_t ignore_step;

  SeqTrackCond() { reset(); }

  ALWAYS_INLINE() void reset() {
    cur_event_idx = 0;
    iterations_5 = 1;
    iterations_6 = 1;
    iterations_7 = 1;
    iterations_8 = 1;
    SeqTrack::reset();
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

      // Iteration counters are contiguous uint8_t members, so walk them as an array to save code size.
      uint8_t *iteration = &iterations_5;
      for (uint8_t max = 5; max <= 8; max++, iteration++) {
        uint8_t value = *iteration + 1;
        if (value > max) {
          value = 1;
        }
        *iteration = value;
      }
    } else {
      step_count++;
    }
  }
  virtual void set_length(uint8_t len, bool expand = false) = 0;
  virtual void clear_track(bool locks = true) = 0;
  virtual void rotate_left() = 0;
  virtual void rotate_right() = 0;
  virtual void reverse() = 0;
  virtual void transpose(int8_t offset) = 0;

  bool conditional(uint8_t condition);
};

class SeqSlideTrack : public SeqTrackCond {
public:
  SlideData locks_slide_data[NUM_LOCKS];
  uint8_t locks_slide_next_lock_val[NUM_LOCKS];
  uint8_t locks_slide_next_lock_step[NUM_LOCKS];
  uint8_t locks_slides_recalc = 255;
  uint16_t locks_slides_idx = 0;

  static constexpr uint8_t number_midi_cc = 6 * 2 + 4;
  static constexpr uint8_t midi_cc_array_size = 6 * 2 + 4;

  ALWAYS_INLINE() void reset() {
    for (uint8_t n = 0; n < NUM_LOCKS; n++) {
      locks_slide_data[n].init();
    }
    SeqTrackCond::reset();
}


  void prepare_slide(uint8_t lock_idx, int16_t x0, int16_t x1, int8_t y0,
                     int8_t y1);
  virtual void send_slides(volatile uint8_t *locks_params, uint8_t channel = 0);

protected:
  virtual void on_slide_dispatch_begin(uint8_t channel);
  virtual void dispatch_slide_value(uint8_t param, uint8_t value,
                                    uint8_t channel);
  virtual void on_slide_dispatch_end();
};

#endif /* SEQTRACK_H__ */
