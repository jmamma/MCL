#include "MidiSeqTrackData.h"

#ifdef PLATFORM_TBD

#include "ExtSeqTrack.h"
#include "GridLink.h"
#include <string.h>

namespace {

uint8_t legacy_ticks_for_speed(uint8_t speed) {
  switch (speed) {
  case SEQ_SPEED_2X:   return 6;
  case SEQ_SPEED_4X:   return 3;
  case SEQ_SPEED_3_4X: return 16;
  case SEQ_SPEED_3_2X: return 8;
  case SEQ_SPEED_1_2X: return 24;
  case SEQ_SPEED_1_4X: return 48;
  case SEQ_SPEED_1_8X: return 96;
  default:
  case SEQ_SPEED_1X:   return 12;
  }
}

uint8_t lock_type_for_legacy_param(uint8_t param) {
  switch (param) {
  case PARAM_PB:  return MIDI_SEQ_LOCK_PITCH_BEND;
  case PARAM_CHP: return MIDI_SEQ_LOCK_CHANNEL_PRESSURE;
  case PARAM_PRG: return MIDI_SEQ_LOCK_PROGRAM_CHANGE;
  default:        return MIDI_SEQ_LOCK_CC;
  }
}

uint16_t value14_from_value7(uint8_t value7) {
  if (value7 > 127) value7 = 127;
  return (uint16_t)(((uint32_t)value7 * 0x3FFFu + 63u) / 127u);
}

} // namespace

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

void MidiSeqTrackData::import_legacy_ext(const ExtSeqTrackData &legacy,
                                         const GridLink &link) {
  clear();
  channel = legacy.channel;
  length = link.length ? link.length : 16;
  speed = link.speed;

  const uint8_t legacy_tps = legacy_ticks_for_speed(link.speed);
  uint16_t idx = 0;
  for (uint8_t step = 0; step < NUM_EXT_STEPS; step++) {
    uint8_t bucket =
        const_cast<ExtSeqTrackData &>(legacy).event_buckets.get(step);
    for (uint8_t i = 0; i < bucket; i++, idx++) {
      const auto &legacy_event = legacy.events[idx];
      int16_t legacy_offset = (int16_t)legacy_event.micro_timing - legacy_tps;
      int16_t data_offset =
          ((int32_t)legacy_offset * MIDI_SEQ_TIMING_CENTER) / legacy_tps;

      MidiSeqEvent event;
      if (legacy_event.is_lock) {
        uint8_t lock_idx = legacy_event.lock_idx;
        if (lock_idx >= MIDI_SEQ_NUM_LOCKS ||
            legacy.locks_params[lock_idx] == 0) {
          continue;
        }
        uint8_t param = legacy.locks_params[lock_idx] - 1;
        locks[lock_idx].init(lock_type_for_legacy_param(param), param, 0, 0);
        event.init(MIDI_SEQ_EVENT_LOCK, lock_idx,
                   value14_from_value7(legacy_event.event_value),
                   MIDI_SEQ_TIMING_CENTER, legacy_event.cond_id,
                   legacy_event.event_on ? MIDI_SEQ_EVENT_FLAG_SLIDE : 0);
      } else {
        event.init(legacy_event.event_on ? MIDI_SEQ_EVENT_NOTE_ON
                                         : MIDI_SEQ_EVENT_NOTE_OFF,
                   legacy_event.event_value,
                   legacy_event.event_on ? legacy.velocities[step] : 0,
                   MIDI_SEQ_TIMING_CENTER, legacy_event.cond_id);
      }
      event.set_microtiming(data_offset);
      add_event(step, event);
    }
  }
  clean_locks();
}

#endif // PLATFORM_TBD
