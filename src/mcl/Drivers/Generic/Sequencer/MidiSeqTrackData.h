#pragma once

#include "MCLMemory.h"
#include "platform.h"
#include <stdint.h>
#include <string.h>

#ifdef PLATFORM_TBD

#define MIDI_SEQ_DATA_VERSION 1

#define MIDI_SEQ_NUM_STEPS 128
#define MIDI_SEQ_NUM_EVENTS 545
#define MIDI_SEQ_NUM_LOCKS 32

// Canonical editor/storage resolution for enhanced external tracks.
// 96 ticks per 16th note is 8x the current 1X MCL scheduler resolution and
// maps cleanly to MIDI-clock PPQN. Playback backends can quantize from here
// or pass the full position to a timestamp-capable target.
#define MIDI_SEQ_TICKS_PER_STEP 96
#define MIDI_SEQ_TIMING_CENTER MIDI_SEQ_TICKS_PER_STEP
#define MIDI_SEQ_TIMING_RANGE (MIDI_SEQ_TICKS_PER_STEP * 2)

enum MidiSeqEventType : uint8_t {
  MIDI_SEQ_EVENT_NOTE_OFF = 0,
  MIDI_SEQ_EVENT_LOCK = 1,
  MIDI_SEQ_EVENT_NOTE_ON = 2,
};

enum MidiSeqLockType : uint8_t {
  MIDI_SEQ_LOCK_OFF = 0,
  MIDI_SEQ_LOCK_CC = 1,
  MIDI_SEQ_LOCK_NRPN = 2,
  MIDI_SEQ_LOCK_RPN = 3,
  MIDI_SEQ_LOCK_PITCH_BEND = 4,
  MIDI_SEQ_LOCK_CHANNEL_PRESSURE = 5,
  MIDI_SEQ_LOCK_PROGRAM_CHANGE = 6,
  MIDI_SEQ_LOCK_POLY_PRESSURE = 7,
};

enum MidiSeqEventFlag : uint8_t {
  MIDI_SEQ_EVENT_FLAG_SLIDE = 1 << 0,
};

enum MidiSeqLockFlag : uint8_t {
  MIDI_SEQ_LOCK_FLAG_14BIT = 1 << 0,
};

#pragma pack(push, 1)

class MidiSeqEvent {
public:
  // Encoded timing is centered at MIDI_SEQ_TIMING_CENTER. A timing
  // value of 96 means on-grid. 0..95 fires before the nominal step, 97..191
  // fires after it.
  uint8_t timing;
  uint8_t type : 2;
  uint8_t condition : 6;
  uint8_t target : 7;
  uint8_t flags_low : 1;
  uint16_t value : 14;
  uint16_t flags_high : 2;

  void init(uint8_t type_, uint8_t target_, uint16_t value_,
            uint8_t timing_ = MIDI_SEQ_TIMING_CENTER,
            uint8_t condition_ = 0, uint8_t flags_ = 0) {
    timing = timing_ >= MIDI_SEQ_TIMING_RANGE
                 ? MIDI_SEQ_TIMING_RANGE - 1
                 : timing_;
    type = type_ & 0x03;
    condition = condition_ & 0x3F;
    target = target_ & 0x7F;
    value = value_ & 0x3FFF;
    set_flags(flags_);
  }

  int16_t microtiming() const {
    return (int16_t)timing - (int16_t)MIDI_SEQ_TIMING_CENTER;
  }

  void set_microtiming(int16_t offset) {
    if (offset < -(int16_t)MIDI_SEQ_TIMING_CENTER) {
      offset = -(int16_t)MIDI_SEQ_TIMING_CENTER;
    } else if (offset >= (int16_t)MIDI_SEQ_TIMING_CENTER) {
      offset = MIDI_SEQ_TIMING_CENTER - 1;
    }
    timing = (uint8_t)((int16_t)MIDI_SEQ_TIMING_CENTER + offset);
  }

  uint8_t flags() const { return flags_low | (flags_high << 1); }

  void set_flags(uint8_t flags_) {
    flags_low = flags_ & 1;
    flags_high = (flags_ >> 1) & 3;
  }

  bool operator<(const MidiSeqEvent &that) const {
    if (timing != that.timing) return timing < that.timing;
    if (type != that.type) return type < that.type;
    return target < that.target;
  }
};

class MidiSeqLockDefinition {
public:
  uint16_t parameter;
  uint16_t default_value;
  uint8_t type : 3;
  uint8_t flags : 5;

  void clear() {
    parameter = 0;
    default_value = 0;
    type = MIDI_SEQ_LOCK_OFF;
    flags = 0;
  }

  void init(uint8_t type_, uint16_t parameter_, uint16_t default_value_ = 0,
            uint8_t flags_ = 0) {
    parameter = parameter_;
    default_value = default_value_ & 0x3FFF;
    type = type_ & 0x07;
    flags = flags_ & 0x1F;
  }

  bool is_active() const { return type != MIDI_SEQ_LOCK_OFF; }
};

class MidiSeqTrackData {
public:
  uint8_t version;
  uint8_t flags;
  uint8_t channel;
  uint8_t length;
  uint8_t speed;
  uint8_t reserved0;
  uint16_t event_count;
  uint8_t lock_count;
  uint8_t reserved[7];

  uint8_t event_buckets[MIDI_SEQ_NUM_STEPS];
  MidiSeqLockDefinition locks[MIDI_SEQ_NUM_LOCKS];
  MidiSeqEvent events[MIDI_SEQ_NUM_EVENTS];

  const uint8_t *data() const { return reinterpret_cast<const uint8_t *>(this); }
  uint8_t *data() { return reinterpret_cast<uint8_t *>(this); }
  static constexpr size_t dataSize() { return sizeof(MidiSeqTrackData); }

  void clear() {
    memset(this, 0, sizeof(MidiSeqTrackData));
    version = MIDI_SEQ_DATA_VERSION;
    length = 16;
  }

  uint16_t locate_start(uint8_t step) const {
    uint16_t idx = 0;
    for (uint8_t i = 0; i < step && i < MIDI_SEQ_NUM_STEPS; i++) {
      idx += event_buckets[i];
    }
    return idx;
  }

  void locate(uint8_t step, uint16_t &start_idx, uint16_t &end_idx) const {
    if (step >= MIDI_SEQ_NUM_STEPS) {
      start_idx = event_count;
      end_idx = event_count;
      return;
    }
    start_idx = locate_start(step);
    end_idx = start_idx + event_buckets[step];
    if (end_idx > event_count) {
      end_idx = event_count;
    }
  }

  uint16_t add_event(uint8_t step, const MidiSeqEvent &event);
  void remove_event(uint16_t index);
  bool clear_step(uint8_t step);

  uint8_t find_lock_idx(uint8_t type, uint16_t parameter) const;
  uint8_t find_or_create_lock(uint8_t type, uint16_t parameter,
                              uint16_t default_value = 0, uint8_t flags = 0);
  void clean_locks();

  uint16_t find_note_event(uint8_t step, uint8_t note, bool note_on,
                           uint16_t &start_idx) const;
  uint16_t find_lock_event(uint8_t step, uint8_t lock_idx,
                           uint16_t &start_idx) const;
  uint8_t search_note_off(uint8_t note, uint8_t step, uint16_t &event_idx,
                          uint16_t event_end, uint8_t search_length = 0) const;

  bool add_note_event(uint8_t step, int16_t microtiming, uint8_t note,
                      bool note_on, uint8_t velocity = 100,
                      uint8_t condition = 0);
  bool add_lock_event(uint8_t step, int16_t microtiming, uint8_t lock_idx,
                      uint16_t value, uint8_t condition = 0,
                      uint8_t flags = 0);

  bool has_data() const { return event_count > 0; }
};

#pragma pack(pop)

static_assert(sizeof(MidiSeqEvent) == 5,
              "MidiSeqEvent layout changed");
static_assert(sizeof(MidiSeqLockDefinition) == 5,
              "MidiSeqLockDefinition layout changed");
static_assert(sizeof(MidiSeqTrackData) <= GRID_SLOT_BYTES,
              "MidiSeqTrackData must fit in one grid slot");

#endif // PLATFORM_TBD
