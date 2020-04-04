/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef EXTSEQTRACK_H__
#define EXTSEQTRACK_H__

#include "MidiUartParent.hh"
//#include "MidiUart.h"
#include "WProgram.h"
#define SEQ_NOTEBUF_SIZE 8
#define SEQ_MUTE_ON 1
#define SEQ_MUTE_OFF 0

#define UART2_PORT 2

class ExtSeqTrackData {
public:
  uint8_t length;
  uint8_t resolution; // Resolution = 2 / ExtPatternResolution
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
  uint8_t step_count;
  uint32_t start_step;
  uint8_t iterations;
  bool mute_until_start = false;

  ALWAYS_INLINE() void seq() {
    if (mute_until_start) {

      if (clock_diff(MidiClock.div16th_counter, start_step) == 0) {
        step_count = 0;
        mute_until_start = false;
      }
    }
    if ((MidiUart2.uart_block == 0) && (mute_until_start == false) &&
        (mute_state == SEQ_MUTE_OFF)) {

      uint8_t timing_counter = MidiClock.mod12_counter;

      if ((resolution == 1)) {
        if (MidiClock.mod12_counter < 6) {
          timing_counter = MidiClock.mod12_counter;
        } else {
          timing_counter = MidiClock.mod12_counter - 6;
        }
      }

      uint8_t next_step = 0;
      if (step_count == length) {
        next_step = 0;
      } else {
        next_step = step_count + 1;
      }

      uint8_t timing_mid = 6 * resolution;
      for (uint8_t c = 0; c < 4; c++) {
        if ((timing[step_count] >= timing_mid) &&
            ((timing[step_count] - timing_mid) == timing_counter)) {

          if (notes[c][step_count] < 0) {
            note_off(abs(notes[c][step_count]) - 1);
          }

          else if (notes[c][step_count] > 0) {
            noteon_conditional(conditional[step_count],
                               abs(notes[c][step_count]) - 1);
          }
        }

        if ((timing[next_step] < timing_mid) &&
            ((timing[next_step]) == timing_counter)) {

          if (notes[c][next_step] < 0) {
            note_off(abs(notes[c][next_step]) - 1);
          } else if (notes[c][next_step] > 0) {
            noteon_conditional(conditional[next_step],
                               abs(notes[c][next_step]) - 1);
          }
        }
      }
    }
    if (((MidiClock.mod12_counter == 11) || (MidiClock.mod12_counter == 5)) &&
        (resolution == 1)) {
      step_count++;
    } else if ((MidiClock.mod12_counter == 11) && (resolution == 2)) {
      step_count++;
    }
    if (step_count == length) {
      step_count = 0;
      iterations++;
      if (iterations > 8) {
        iterations = 1;
      }
    }
  }

  ALWAYS_INLINE() void note_on(uint8_t note) {
    uart->sendNoteOn(channel, note, 100);
    // Greater than 64
    if (IS_BIT_SET(note, 6)) {
      SET_BIT64(note_buffer[1], note - 64);
    } else {
      SET_BIT64(note_buffer[0], note);
    }
  }

  ALWAYS_INLINE() void note_off(uint8_t note) {
    uart->sendNoteOff(channel, note, 0);

    // Greater than 64
    if (IS_BIT_SET(note, 6)) {
      CLEAR_BIT64(note_buffer[1], note - 64);
    } else {
      CLEAR_BIT64(note_buffer[0], note);
    }
  }

  ALWAYS_INLINE() void noteon_conditional(uint8_t condition, uint8_t note) {
    switch (condition) {
    case 0:
      note_on(note);
      break;
    case 1:
      note_on(note);
      break;
    case 2:
      if (!IS_BIT_SET(iterations, 0)) {
        note_on(note);
      }
    case 4:
      if ((iterations == 4) || (iterations == 8)) {
        note_on(note);
      }
    case 8:
      if ((iterations == 8)) {
        note_on(note);
      }
      break;
    case 3:
      if ((iterations == 3) || (iterations == 6)) {
        note_on(note);
      }
      break;
    case 5:
      if (iterations == 5) {
        note_on(note);
      }
      break;
    case 7:
      if (iterations == 7) {
        note_on(note);
      }
      break;
    case 9:
      if (get_random_byte() <= 13) {
        note_on(note);
      }
      break;
    case 10:
      if (get_random_byte() <= 32) {
        note_on(note);
      }
      break;
    case 11:
      if (get_random_byte() <= 64) {
        note_on(note);
      }
      break;
    case 12:
      if (get_random_byte() <= 96) {
        note_on(note);
      }
      break;
    case 13:
      if (get_random_byte() <= 115) {
        note_on(note);
      }
      break;
    }
  }

  void record_ext_track_noteon(uint8_t note_num, uint8_t velocity);
  void record_ext_track_noteoff(uint8_t note_num, uint8_t velocity);

  void set_ext_track_step(uint8_t step, uint8_t note_num, uint8_t velocity);

  void clear_ext_conditional();
  void clear_ext_notes();
  void clear_track();
  void set_length(uint8_t len);

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

  void rotate_left();
  void rotate_right();
  void reverse();
};

#endif /* EXTSEQTRACK_H__ */
