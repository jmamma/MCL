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

  uint64_t note_buffer[2] = {0}; // 2 x 64 bit masks to store state of 128 notes.
  uint64_t oneshot_mask;

  uint8_t step_count;
  uint32_t start_step;
  bool mute_until_start = false;

  //Conditional counters
  uint8_t iterations_5;
  uint8_t iterations_6;
  uint8_t iterations_7;
  uint8_t iterations_8;

  ALWAYS_INLINE() void seq();
  ALWAYS_INLINE() void set_step(uint8_t step, uint8_t note_num, uint8_t velocity);
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

  void buffer_notesoff() {
    buffer_notesoff64(&(note_buffer[0]),0);
    buffer_notesoff64(&(note_buffer[1]),64);
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
