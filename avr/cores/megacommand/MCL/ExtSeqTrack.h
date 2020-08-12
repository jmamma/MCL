/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef EXTSEQTRACK_H__
#define EXTSEQTRACK_H__

#include "MidiUartParent.hh"
//#include "MidiUart.h"
#include "SeqTrack.h"
#include "WProgram.h"

#define NUM_EXT_STEPS 128
#define NUM_EXT_NOTES 4
#define NUM_EXT_LOCKS 4

#define SEQ_NOTEBUF_SIZE 8
#define SEQ_MUTE_ON 1
#define SEQ_MUTE_OFF 0

#define UART2_PORT 2

// EXT Track speed
#define EXT_SPEED_270_1X 2
#define EXT_SPEED_270_2X 1
#define EXT_SPEED_270_3_4X 3
#define EXT_SPEED_270_3_2X 4
#define EXT_SPEED_270_1_2X 5
#define EXT_SPEED_270_1_4X 6
#define EXT_SPEED_270_1_8X 7
#define NUM_EXT_NOTES_270 4
#define NUM_EXT_LOCKS_270 4
#define NUM_EXT_STEPS_270 128

#define EXTSEQTRACKDATA_VERSION 30

class ExtSeqTrackData_270 {
public:
  uint8_t length; // Resolution = 2 / ExtPatternResolution
  uint8_t speed;
  uint8_t reserved[NUM_EXT_NOTES_270];
  int8_t notes[NUM_EXT_NOTES_270]
              [NUM_EXT_STEPS_270]; // 128 steps, up to 4 notes per step
  uint8_t locks[NUM_EXT_LOCKS_270][NUM_EXT_STEPS_270];
  uint8_t locks_params[NUM_EXT_LOCKS_270];
  uint64_t lock_masks[NUM_EXT_LOCKS_270];

  uint8_t conditional[NUM_EXT_STEPS_270];
  uint8_t timing[NUM_EXT_STEPS_270];
};

class ExtSeqTrackData {
public:
  uint8_t version;
  int8_t notes[NUM_EXT_NOTES]
              [NUM_EXT_STEPS]; // 128 steps, up to 4 notes per step

  uint8_t notes_timing[NUM_EXT_NOTES][NUM_EXT_STEPS];
  uint8_t notes_conditional[NUM_EXT_NOTES][NUM_EXT_STEPS];

  uint8_t locks_params[NUM_EXT_LOCKS];
  uint64_t locks_masks[NUM_EXT_LOCKS][2]; // 128bit

  uint8_t locks[NUM_EXT_LOCKS][NUM_EXT_STEPS];
  bool convert(ExtSeqTrackData_270 *old) {
    /*ordering of these statements is important to ensure memory
     * is copied before being overwritten*/
    version = EXTSEQTRACKDATA_VERSION;
    memcpy(&notes, old->notes, NUM_EXT_NOTES_270 * NUM_EXT_STEPS_270);
    for (uint8_t a = 0; a < NUM_EXT_NOTES; a++) {
      memcpy(&notes_timing[a][0], old->timing, NUM_EXT_STEPS_270);
      memcpy(&notes_conditional[a][0], old->conditional, NUM_EXT_STEPS_270);
    }
    memset(&locks_params, 0, NUM_EXT_LOCKS);
    memset(&locks_masks, 0, NUM_EXT_LOCKS * 2);
    return true;
  }
};
class ExtSeqTrack : public ExtSeqTrackData, SeqTrack {

public:
  uint64_t note_buffer[2] = {
      0}; // 2 x 64 bit masks to store state of 128 notes.
  uint64_t oneshot_mask[2];

  ALWAYS_INLINE() void reset() {
    SeqTrack::reset();
    oneshot_mask[0] = 0;
    oneshot_mask[1] = 0;
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
  void re_sync();

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
};

#endif /* EXTSEQTRACK_H__ */
