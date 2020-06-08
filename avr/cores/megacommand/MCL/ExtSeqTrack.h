/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef EXTSEQTRACK_H__
#define EXTSEQTRACK_H__

#include "MidiUartParent.hh"
//#include "MidiUart.h"
#include "SeqTrack.h"
#include "WProgram.h"
#define SEQ_NOTEBUF_SIZE 8
#define SEQ_MUTE_ON 1
#define SEQ_MUTE_OFF 0

#define UART2_PORT 2

// EXT Track speed
#define EXT_SPEED_1X 2
#define EXT_SPEED_2X 1
#define EXT_SPEED_3_4X 3
#define EXT_SPEED_3_2X 4
#define EXT_SPEED_1_2X 5
#define EXT_SPEED_1_4X 6
#define EXT_SPEED_1_8X 7

const uint8_t ext_speeds[7] PROGMEM = {
    EXT_SPEED_1X,   EXT_SPEED_2X,   EXT_SPEED_3_4X, EXT_SPEED_3_2X,
    EXT_SPEED_1_2X, EXT_SPEED_1_4X, EXT_SPEED_1_8X};

class ExtSeqTrackData {
public:
  uint8_t length;
  uint8_t speed; // Resolution = 2 / ExtPatternResolution
  uint8_t reserved[4];
  int8_t notes[4][128]; // 128 steps, up to 4 notes per step
  uint8_t locks[4][128];
  uint8_t locks_params[4];
  uint64_t lock_masks[4];

  uint8_t conditional[128];
  uint8_t timing[128];
};
class ExtSeqTrack : public ExtSeqTrackData {

public:
  uint8_t channel;
  uint8_t port = UART2_PORT;
  MidiUartParent *uart = &MidiUart2;

  uint8_t mute_state = SEQ_MUTE_OFF;

  uint64_t note_buffer[2] = {
      0}; // 2 x 64 bit masks to store state of 128 notes.
  uint64_t oneshot_mask[2];

  uint8_t step_count;
  uint8_t mod12_counter;
  uint32_t start_step;
  uint8_t start_step_offset;
  bool mute_until_start = false;

  // Conditional counters
  uint8_t iterations_5;
  uint8_t iterations_6;
  uint8_t iterations_7;
  uint8_t iterations_8;

  ALWAYS_INLINE() void reset() {
    step_count = 0;
    oneshot_mask[0] = 0;
    oneshot_mask[1] = 0;
    mute_until_start = false;
    iterations_5 = 1;
    iterations_6 = 1;
    iterations_7 = 1;
    iterations_8 = 1;
  }
  ALWAYS_INLINE() void step_count_inc() {
    if (step_count == length - 1) {
      step_count = 0;

      iterations_5++;
      iterations_6++;
      iterations_7++;
      iterations_8++;

      if (iterations_5 > 5) {
        iterations_5 = 1;
      } 
      if (iterations_6 > 6) {
        iterations_8 = 1;
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
  ALWAYS_INLINE() void seq();
  ALWAYS_INLINE()
  void set_step(uint8_t step, uint8_t note_num, uint8_t velocity);
  ALWAYS_INLINE() void note_on(uint8_t note);
  ALWAYS_INLINE() void note_off(uint8_t note);
  ALWAYS_INLINE() void noteon_conditional(uint8_t condition, uint8_t note);

  void record_ext_track_noteon(uint8_t note_num, uint8_t velocity);
  void record_ext_track_noteoff(uint8_t note_num, uint8_t velocity);

  void set_ext_track_step(uint8_t step, uint8_t note_num, uint8_t velocity);

  void clear_ext_conditional();
  void clear_ext_notes();
  void clear_track();
  void set_length(uint8_t len);
  ALWAYS_INLINE() uint8_t get_timing_mid() {
    uint8_t timing_mid;
    switch (speed) {
    default:
    case EXT_SPEED_1X:
      timing_mid = 12;
      break;
    case EXT_SPEED_2X:
      timing_mid = 6;
      break;
    case EXT_SPEED_3_4X:
      timing_mid = 16; // 12 * (4.0/3.0);
      break;
    case EXT_SPEED_3_2X:
      timing_mid = 8; // 12 * (2.0/3.0);
      break;
    case EXT_SPEED_1_2X:
      timing_mid = 24;
      break;
    case EXT_SPEED_1_4X:
      timing_mid = 48;
      break;
    case EXT_SPEED_1_8X:
      timing_mid = 96;
      break;
    }
    return timing_mid;
  }
  void buffer_notesoff() {
    buffer_notesoff64(&(note_buffer[0]), 0);
    buffer_notesoff64(&(note_buffer[1]), 64);
  }

  void buffer_notesoff64(uint64_t *buf, uint8_t offset) {
    buffer_notesoff32(&(((uint32_t *)buf)[0]), offset);
    buffer_notesoff32(&(((uint32_t *)buf)[1]), offset + 32);
  }

  void buffer_notesoff32(uint32_t *buf, uint8_t offset) {
    buffer_notesoff16(&(((uint16_t *)buf)[0]), offset);
    buffer_notesoff16(&(((uint16_t *)buf)[1]), offset + 16);
  }

  void buffer_notesoff16(uint16_t *buf, uint8_t offset) {
    if (((uint8_t *)buf)[0]) {
      buffer_notesoff8(&(((uint8_t *)buf)[0]), offset);
    }
    if (((uint8_t *)buf)[1]) {
      buffer_notesoff8(&(((uint8_t *)buf)[1]), offset + 8);
    }
  }
  void buffer_notesoff8(uint8_t *buf, uint8_t offset) {
    if (IS_BIT_SET(*buf, 0)) {
      uart->sendNoteOff(channel, offset, 0);
    }
    if (IS_BIT_SET(*buf, 1)) {
      uart->sendNoteOff(channel, offset + 1, 0);
    }
    if (IS_BIT_SET(*buf, 2)) {
      uart->sendNoteOff(channel, offset + 2, 0);
    }
    if (IS_BIT_SET(*buf, 3)) {
      uart->sendNoteOff(channel, offset + 3, 0);
    }
    if (IS_BIT_SET(*buf, 4)) {
      uart->sendNoteOff(channel, offset + 4, 0);
    }
    if (IS_BIT_SET(*buf, 5)) {
      uart->sendNoteOff(channel, offset + 5, 0);
    }
    if (IS_BIT_SET(*buf, 6)) {
      uart->sendNoteOff(channel, offset + 6, 0);
    }
    if (IS_BIT_SET(*buf, 7)) {
      uart->sendNoteOff(channel, offset + 7, 0);
    }
    *buf = 0;
  }
  #define DIR_LEFT 0
  #define DIR_RIGHT 1
  #define DIR_REVERSE 2

  void rotate_left() { modify_track(DIR_LEFT); }
  void rotate_right() { modify_track(DIR_RIGHT); }
  void reverse() { modify_track(DIR_REVERSE); }

  void modify_track(uint8_t dir);

  void set_speed(uint8_t _speed);
  float get_speed_multiplier();
  float get_speed_multiplier(uint8_t speed);
};

#endif /* EXTSEQTRACK_H__ */
