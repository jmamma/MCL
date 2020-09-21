/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef EXTSEQTRACK_H__
#define EXTSEQTRACK_H__

#include "MidiUartParent.h"
//#include "MidiUart.h"
#include "SeqTrack.h"
#include "WProgram.h"
#include "CommonTools/NibbleArray.h"
#include "MCL.h"

#define NUM_EXT_STEPS 128
#define NUM_EXT_EVENTS 512
#define NUM_EXT_LOCKS 8

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

/// 24-bit ext track event descriptor
struct ext_event_t {
  /// true for lock, false for Midi note
  bool is_lock : 1;
  /// effective when is_lock is true
  uint8_t lock_idx : 3;
  uint8_t cond_id : 4;
  /// for Midi note: note on/off
  /// for plock: lock engage/disengage
  bool event_on : 1;
  /// for Midi note: pitch value
  /// for plock & lock engage: lock value
  /// for plock & lock disengage: ignored
  uint8_t event_value : 7;
  /// micro timing value
  uint8_t micro_timing;
};

class ExtSeqTrackData {
public:
  NibbleArray<128> timing_buckets;
  ext_event_t events[NUM_EXT_EVENTS];
  uint8_t locks_params[NUM_EXT_LOCKS];
  uint16_t event_count;

  bool convert(ExtSeqTrackData_270 *old) {
    // TODO
    return false;
  }

  void reset() {
    event_count = 0;
    timing_buckets.clear();
  }
};

class ExtSeqTrack : public ExtSeqTrackData, public SeqTrack {

public:
  uint64_t note_buffer[2] = {0}; // 2 x 64 bit masks to store state of 128 notes.
  uint64_t oneshot_mask[2];

  ALWAYS_INLINE() void reset() {
    SeqTrack::reset();
    oneshot_mask[0] = 0;
    oneshot_mask[1] = 0;
    ExtSeqTrackData::reset();
  }

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
  void re_sync();
  void handle_event(uint16_t index);
  void remove_event(uint16_t index);
  uint16_t add_event(uint8_t step);
  uint16_t find_midi_note(uint8_t step, uint8_t note_num, uint16_t& ev_idx);

  void locate(uint8_t step, uint16_t& ev_idx, uint16_t& ev_end) {
    ev_idx = 0;
    ev_end = timing_buckets.get(step_count);
    for (uint8_t i = 0; i < step_count; ++i) {
      ev_idx += timing_buckets.get(i);
    }
    ev_end += ev_idx;
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

  void rotate_left() { modify_track(DIR_LEFT); }
  void rotate_right() { modify_track(DIR_RIGHT); }
  void reverse() { modify_track(DIR_REVERSE); }

  void modify_track(uint8_t dir);

  void set_speed(uint8_t _speed);
};

#endif /* EXTSEQTRACK_H__ */
