#include "MidiSeqTrackData.h"

#ifdef PLATFORM_TBD

#include <string.h>

uint16_t MidiSeqTrackData::add_event(
    uint8_t step, const MidiSeqEvent &event) {
  if (step >= MIDI_SEQ_NUM_STEPS ||
      event_count >= MIDI_SEQ_NUM_EVENTS ||
      event_buckets[step] == 0xFF) {
    return 0xFFFF;
  }

  uint16_t idx = 0;
  uint16_t end = 0;
  locate(step, idx, end);

  while (idx < end && events[idx] < event) {
    idx++;
  }

  if (idx < event_count) {
    memmove(events + idx + 1, events + idx,
            sizeof(MidiSeqEvent) * (event_count - idx));
  }

  events[idx] = event;
  event_buckets[step]++;
  event_count++;
  return idx;
}

void MidiSeqTrackData::remove_event(uint16_t index) {
  if (index >= event_count) {
    return;
  }

  uint16_t start = 0;
  for (uint8_t step = 0; step < MIDI_SEQ_NUM_STEPS; step++) {
    uint8_t bucket = event_buckets[step];
    if (index < start + bucket) {
      event_buckets[step] = bucket - 1;
      break;
    }
    start += bucket;
  }

  if (index + 1 < event_count) {
    memmove(events + index, events + index + 1,
            sizeof(MidiSeqEvent) * (event_count - index - 1));
  }
  event_count--;
}

bool MidiSeqTrackData::clear_step(uint8_t step) {
  if (step >= MIDI_SEQ_NUM_STEPS) {
    return false;
  }

  uint16_t start = 0;
  uint16_t end = 0;
  locate(step, start, end);
  uint8_t bucket = event_buckets[step];
  if (bucket == 0) {
    return false;
  }

  if (end < event_count) {
    memmove(events + start, events + end,
            sizeof(MidiSeqEvent) * (event_count - end));
  }

  event_count -= bucket;
  event_buckets[step] = 0;
  return true;
}

uint8_t MidiSeqTrackData::find_lock_idx(uint8_t type,
                                        uint16_t parameter,
                                        uint8_t flags) const {
  for (uint8_t i = 0; i < MIDI_SEQ_NUM_LOCKS; i++) {
    if (locks[i].is_active() && locks[i].type == (type & 0x07) &&
        locks[i].parameter == parameter && locks[i].flags == (flags & 0x1F)) {
      return i;
    }
  }
  return 255;
}

uint8_t MidiSeqTrackData::find_or_create_lock(uint8_t type,
                                              uint16_t parameter,
                                              uint16_t default_value,
                                              uint8_t flags) {
  uint8_t idx = find_lock_idx(type, parameter, flags);
  if (idx != 255) {
    return idx;
  }

  for (uint8_t i = 0; i < MIDI_SEQ_NUM_LOCKS; i++) {
    if (!locks[i].is_active()) {
      locks[i].init(type, parameter, default_value, flags);
      lock_count++;
      return i;
    }
  }
  return 255;
}

void MidiSeqTrackData::clean_locks() {
  bool used[MIDI_SEQ_NUM_LOCKS] = {};

  for (uint16_t i = 0; i < event_count; i++) {
    if (events[i].type == MIDI_SEQ_EVENT_LOCK &&
        events[i].target < MIDI_SEQ_NUM_LOCKS) {
      used[events[i].target] = true;
    }
  }

  lock_count = 0;
  for (uint8_t i = 0; i < MIDI_SEQ_NUM_LOCKS; i++) {
    if (used[i]) {
      if (locks[i].is_active()) {
        lock_count++;
      }
    } else {
      locks[i].clear();
    }
  }
}

uint16_t MidiSeqTrackData::find_note_event(uint8_t step, uint8_t note,
                                           bool note_on,
                                           uint16_t &start_idx) const {
  uint16_t end = 0;
  locate(step, start_idx, end);
  const uint8_t type = note_on ? MIDI_SEQ_EVENT_NOTE_ON
                               : MIDI_SEQ_EVENT_NOTE_OFF;

  for (uint16_t i = start_idx; i < end; i++) {
    if (events[i].type == type && events[i].target == note) {
      return i;
    }
  }
  return 0xFFFF;
}

uint16_t MidiSeqTrackData::find_lock_event(uint8_t step, uint8_t lock_idx,
                                           uint16_t &start_idx) const {
  uint16_t end = 0;
  locate(step, start_idx, end);

  for (uint16_t i = start_idx; i < end; i++) {
    if (events[i].type == MIDI_SEQ_EVENT_LOCK &&
        events[i].target == lock_idx) {
      return i;
    }
  }
  return 0xFFFF;
}

uint8_t MidiSeqTrackData::search_note_off(uint8_t note, uint8_t step,
                                          uint16_t &event_idx,
                                          uint16_t event_end,
                                          uint8_t search_length) const {
  uint8_t len = search_length;
  if (len == 0 || len > MIDI_SEQ_NUM_STEPS) {
    len = length == 0 ? 16 : length;
  }
  if (len > MIDI_SEQ_NUM_STEPS) {
    len = MIDI_SEQ_NUM_STEPS;
  }
  if (step >= len) {
    event_idx = 0xFFFF;
    return step;
  }

  uint8_t cur_step = step;
  event_idx++;

  do {
    for (; event_idx < event_end; event_idx++) {
      const auto &event = events[event_idx];
      if (event.type == MIDI_SEQ_EVENT_NOTE_OFF &&
          event.target == note) {
        return cur_step;
      }
    }

    cur_step++;
    if (cur_step >= len) {
      cur_step = 0;
      event_end = 0;
    }
    event_idx = event_end;
    event_end += event_buckets[cur_step];
  } while (cur_step != step);

  event_idx = 0xFFFF;
  return step;
}

bool MidiSeqTrackData::add_note_event(uint8_t step, int16_t microtiming,
                                      uint8_t note, bool note_on,
                                      uint8_t velocity, uint8_t condition) {
  if (step >= MIDI_SEQ_NUM_STEPS) {
    return false;
  }

  MidiSeqEvent event;
  event.init(note_on ? MIDI_SEQ_EVENT_NOTE_ON
                     : MIDI_SEQ_EVENT_NOTE_OFF,
             note, velocity, MIDI_SEQ_TIMING_CENTER, condition);
  event.set_microtiming(microtiming);
  return add_event(step, event) != 0xFFFF;
}

bool MidiSeqTrackData::add_lock_event(uint8_t step, int16_t microtiming,
                                      uint8_t lock_idx, uint16_t value,
                                      uint8_t condition, uint8_t flags) {
  if (step >= MIDI_SEQ_NUM_STEPS ||
      lock_idx >= MIDI_SEQ_NUM_LOCKS) {
    return false;
  }

  MidiSeqEvent event;
  event.init(MIDI_SEQ_EVENT_LOCK, lock_idx, value,
             MIDI_SEQ_TIMING_CENTER, condition, flags);
  event.set_microtiming(microtiming);
  return add_event(step, event) != 0xFFFF;
}

#endif // PLATFORM_TBD
