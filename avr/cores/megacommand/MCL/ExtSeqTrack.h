/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef EXTSEQTRACK_H__
#define EXTSEQTRACK_H__

#include "MidiUartParent.h"
//#include "MidiUart.h"
#include "CommonTools/NibbleArray.h"
#include "MCL.h"
#include "SeqTrack.h"
#include "WProgram.h"

#define NUM_EXT_STEPS 128
#define NUM_EXT_EVENTS 512
#define NUM_NOTES_ON 16 // number of notes that can be recorded simultaneously.

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
#define NUM_LOCKS_270 4
#define NUM_EXT_STEPS_270 128

class ExtSeqTrackData_270 {
public:
  uint8_t length; // Resolution = 2 / ExtPatternResolution
  uint8_t speed;
  uint8_t reserved[NUM_EXT_NOTES_270];
  int8_t notes[NUM_EXT_NOTES_270]
              [NUM_EXT_STEPS_270]; // 128 steps, up to 4 notes per step
  uint8_t locks[NUM_LOCKS_270][NUM_EXT_STEPS_270];
  uint8_t locks_params[NUM_LOCKS_270];

  uint64_t lock_masks[NUM_LOCKS_270];

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
  /// for plock: lock slide engage/disengage
  bool event_on : 1;
  /// for Midi note: pitch value
  /// for plock & lock engage: lock value
  /// for plock & lock disengage: ignored
  uint8_t event_value : 7;
  /// micro timing value
  uint8_t micro_timing;

  bool operator<(const ext_event_t &that) {
    // order by micro_timing
    if (this->micro_timing != that.micro_timing) {
      return this->micro_timing < that.micro_timing;
    }
    /*
    // order by lock

    if (this->is_lock != that.is_lock) {
      return this.is_lock;
    }
    */
    // off < on
    if (this->event_on != that.event_on) {
      return that.event_on;
    }

    return false;
  }
};

class NoteVector {
public:
  uint16_t x;
  uint8_t value;
  uint8_t velocity;
};

class ExtSeqTrackData {
public:
  NibbleArray<NUM_EXT_STEPS> timing_buckets;
  ext_event_t events[NUM_EXT_EVENTS];
  uint8_t locks_params[NUM_LOCKS];
  uint16_t event_count;
  uint8_t velocities[NUM_EXT_STEPS];
  uint8_t locks_params_orig[NUM_LOCKS];
  uint8_t channel;
  void set_channel(uint8_t channel_) { channel = channel_; }
  void *data() const { return (void *)&timing_buckets; }
  void clear() {
    event_count = 0;
    timing_buckets.clear();
  }
  bool convert(ExtSeqTrackData_270 *old) {
    clear();
    memset(locks_params, 0, NUM_LOCKS_270);
    for (uint8_t n = 0; n < old->length; n++) {
      for (uint8_t a = 0; a < NUM_EXT_NOTES_270; a++) {
        if (old->notes[a][n] == 0)
          continue;
        bool note_on = (old->notes[a][n] > 0);
        ext_event_t e;

        e.is_lock = false;
        e.cond_id = old->conditional[n];
        e.event_value = abs(old->notes[a][n]) - 1;
        e.event_on = note_on;
        e.micro_timing = old->timing[n];
        velocities[n] = 100;
        if (event_count < NUM_EXT_EVENTS) {
          uint8_t bucket = timing_buckets.get(n);
          if (bucket < 16) {
            timing_buckets.set(n, bucket + 1);
            events[event_count++] = e;
          }
        }
      }
    }
    return true;
  }
};

class ExtSeqTrack : public ExtSeqTrackData, public SeqSlideTrack {

public:
  uint64_t note_buffer[2] = {
      0}; // 2 x 64 bit masks to store state of 128 notes.
  uint64_t oneshot_mask[2];

  NoteVector notes_on[NUM_NOTES_ON];
  uint8_t notes_on_count;

  uint8_t locks_slide_next_lock_utiming[NUM_LOCKS];

  ExtSeqTrack() : SeqSlideTrack() { active = EXT_TRACK_TYPE; }

  ALWAYS_INLINE() void reset() {
    SeqTrack::reset();
    memset(oneshot_mask,0,sizeof(oneshot_mask));
  }

  ALWAYS_INLINE() void seq(MidiUartParent *uart_);
  ALWAYS_INLINE() void set_step(uint8_t step, uint8_t note_num, uint8_t velocity);
  ALWAYS_INLINE() void note_on(uint8_t note, uint8_t velocity = 100, MidiUartParent *uart_ = nullptr);
  ALWAYS_INLINE() void note_off(uint8_t note, uint8_t velocity = 100, MidiUartParent *uart_ = nullptr);
  ALWAYS_INLINE()
  void noteon_conditional(uint8_t condition, uint8_t note,
                          uint8_t velocity = 100);
  void find_next_locks(uint16_t curidx, uint8_t step, uint8_t *find_array);
  uint8_t find_next_lock(uint8_t step, uint8_t lock_idx, uint16_t &curidx,
                         uint16_t &end);

  void update_param(uint8_t param_id, uint8_t value);

  uint8_t find_lock_idx(uint8_t param_id);
  uint16_t find_lock(uint8_t step, uint8_t lock_idx, uint16_t &start_idx);
  uint8_t count_lock_event(uint8_t step, uint8_t lock_idx);

  bool set_track_locks(uint8_t step, uint8_t utiming, uint8_t track_param,
                       uint8_t value, bool slide = true,
                       uint8_t lock_idx = 255);
  void recalc_slides();

  void record_track_locks(uint8_t track_param, uint8_t value,
                          bool slide = false);
  void record_track_noteon(uint8_t note_num, uint8_t velocity);
  void record_track_noteoff(uint8_t note_num);

  bool set_track_step(uint8_t &step, uint8_t utiming, uint8_t note_num,
                      uint8_t event_on, uint8_t velocity, uint8_t cond);
  void clear_ext_conditional();
  void clear_ext_notes();

  // clear_track_locks: if value != 255, delete specific lock event of value.
  // otherwise delete all locks matching track_param of any value
  bool del_track_locks(int16_t cur_x, uint8_t lock_idx, uint8_t value);
  void clear_track_locks(uint8_t idx);
  bool clear_track_locks(uint8_t step, uint8_t track_param,
                         uint8_t value = 255);
  bool clear_track_locks_idx(uint8_t step, uint8_t lock_idx, uint8_t value = 255);
  void clear_track();
  void set_length(uint8_t len);
  void re_sync();
  void reset_params();
  void handle_event(uint16_t index, uint8_t step);
  void remove_event(uint16_t index);
  uint16_t add_event(uint8_t step, ext_event_t *e);

  void init_notes_on();
  void add_notes_on(uint16_t x, uint8_t value, uint8_t velocity);
  uint8_t find_notes_on(uint8_t value);
  void remove_notes_on(uint8_t value);

  bool del_note(uint16_t cur_x, uint16_t cur_w = 0, uint8_t cur_y = 0);
  void add_note(uint16_t cur_x, uint16_t cur_w, uint8_t cur_y, uint8_t velocity,
                uint8_t cond = 0);

  // find midi note within the given step.
  // returns: note index & step start index.
  uint16_t find_midi_note(uint8_t step, uint8_t note_num, uint16_t &start_idx,
                          bool event_on);
  uint16_t find_midi_note(uint8_t step, uint8_t note_num, uint16_t &start_idx);

  // search forward, then wrap around
  // caller pass in note_idx of the note on event, and end index for current
  // bucket. returns: step index & note index
  uint8_t search_note_off(int8_t note_val, uint8_t step, uint16_t &note_idx,
                          uint16_t ev_end);
  uint8_t search_lock_idx(uint8_t lock_idx, uint8_t step, uint16_t &ev_idx,
                          uint16_t &ev_end);
  void locate(uint8_t step, uint16_t &ev_idx, uint16_t &ev_end) {
    ev_idx = 0;
    ev_end = timing_buckets.get(step);
    for (uint8_t i = 0; i < step; ++i) {
      ev_idx += timing_buckets.get(i);
    }

    ev_end += ev_idx;
  }

  void buffer_notesoff() {
    init_notes_on();
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
