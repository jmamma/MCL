#include "MidiSeqTrack.h"

#if !defined(__AVR__)

#include "CommonPages.h"
#include "EmptyTrack.h"
#include "ExtSeqTrack.h"
#include "GridLink.h"
#include "MidiClock.h"
#include "MCLSysConfig.h"
#include "SeqPage.h"
#include "SeqTrackTransition.h"
#include "mcl.h"
#include <string.h>

uint8_t MidiSeqTrack::epoch = 0;

namespace {

uint16_t midi_seq_ticks_for_speed(uint8_t speed) {
  switch (speed) {
  case SEQ_SPEED_2X:   return MIDI_SEQ_TICKS_PER_STEP / 2;
  case SEQ_SPEED_4X:   return MIDI_SEQ_TICKS_PER_STEP / 4;
  case SEQ_SPEED_3_4X: return MIDI_SEQ_TICKS_PER_STEP * 4 / 3;
  case SEQ_SPEED_3_2X: return MIDI_SEQ_TICKS_PER_STEP * 2 / 3;
  case SEQ_SPEED_1_2X: return MIDI_SEQ_TICKS_PER_STEP * 2;
  case SEQ_SPEED_1_4X: return MIDI_SEQ_TICKS_PER_STEP * 4;
  case SEQ_SPEED_1_8X: return MIDI_SEQ_TICKS_PER_STEP * 8;
  default:
  case SEQ_SPEED_1X:   return MIDI_SEQ_TICKS_PER_STEP;
  }
}

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

uint8_t value7_from_value14(uint16_t value14) {
  if (value14 > 0x3FFF) value14 = 0x3FFF;
  return (uint8_t)(((uint32_t)value14 * 127u + 0x1FFFu) / 0x3FFFu);
}

uint16_t value14_from_value7(uint8_t value7) {
  if (value7 > 127) value7 = 127;
  return (uint16_t)(((uint32_t)value7 * 0x3FFFu + 63u) / 127u);
}

uint16_t clamp_value14(int32_t value) {
  if (value < 0) return 0;
  if (value > 0x3FFF) return 0x3FFF;
  return (uint16_t)value;
}

constexpr uint8_t MIDI_SEQ_COND_ONESHOT = 14;

bool current_event_due(uint16_t timing, uint16_t tps, uint16_t tick_counter) {
  if (timing < tps) return false;
  uint16_t trigger_tick = (uint16_t)(timing - tps + 1);
  if (trigger_tick >= tps) trigger_tick = tps - 1;
  return tick_counter == trigger_tick;
}

bool next_event_due(uint16_t timing, uint16_t tps, uint16_t tick_counter) {
  if (timing >= tps) return false;
  uint16_t trigger_tick = timing;
  if (trigger_tick < 1) trigger_tick = 1;
  return tick_counter == trigger_tick;
}

} // namespace

MidiSeqTrack::MidiSeqTrack() : SeqTrackCond() {
  active = MIDI_TRACK_TYPE;
  seq_data.clear();
  length = seq_data.length;
  speed = seq_data.speed;
  init_notes_on();
  for (uint8_t i = 0; i < MIDI_SEQ_NUM_LOCKS; i++) {
    locks_slide_data[i].init();
  }
}

void MidiSeqTrack::reset() {
  SeqTrackCond::reset();
  tick_counter = 0;
  mod12_counter = 0;
  memset(ignore_notes, 0, sizeof(ignore_notes));
  memset(oneshot_mask, 0, sizeof(oneshot_mask));
  cache_loaded = true;
  pending_cache_track_type_ = EMPTY_TRACK_TYPE;
  pending_cache_slot_ = 255;
  locks_slides_recalc = 255;
  locks_slides_idx = 0;
  for (uint8_t i = 0; i < MIDI_SEQ_NUM_LOCKS; i++) {
    locks_slide_data[i].init();
  }
}

void MidiSeqTrack::defer_cache_load(uint8_t track_type, GridSlot slot) {
  pending_cache_track_type_ = track_type;
  pending_cache_slot_ = slot;
  cache_loaded = false;
}

void MidiSeqTrack::load_cache() {
  if (pending_cache_track_type_ == EMPTY_TRACK_TYPE ||
      pending_cache_slot_ == 255) {
    return;
  }

  buffer_notesoff();
  EmptyTrack scratch;
  DeviceTrack *track =
      scratch.load_from_mem(pending_cache_slot_, pending_cache_track_type_);
  if (track != nullptr) {
    track->load_seq_data(this);
  }
  pending_cache_track_type_ = EMPTY_TRACK_TYPE;
  pending_cache_slot_ = 255;
}

uint16_t MidiSeqTrack::ticks_per_step() const {
  uint16_t tps = midi_seq_ticks_for_speed(speed);
  return tps == 0 ? MIDI_SEQ_TICKS_PER_STEP : tps;
}

void MidiSeqTrack::update_legacy_progress_counter() {
  uint16_t tps = ticks_per_step();
  uint8_t legacy_tps = SeqTrack::get_speed_multiplier_int(speed);
  mod12_counter = (tps == 0)
                      ? 0
                      : (uint8_t)(((uint32_t)tick_counter * legacy_tps) / tps);
}

uint16_t MidiSeqTrack::data_timing_to_page(uint8_t timing) const {
  return (uint16_t)(((uint32_t)timing * ticks_per_step() +
                     (MIDI_SEQ_TIMING_CENTER / 2)) /
                    MIDI_SEQ_TIMING_CENTER);
}

uint8_t MidiSeqTrack::page_timing_to_data(uint16_t timing) const {
  uint16_t tps = ticks_per_step();
  uint32_t converted = tps == 0
                           ? MIDI_SEQ_TIMING_CENTER
                           : ((uint32_t)timing * MIDI_SEQ_TIMING_CENTER +
                              (tps / 2)) /
                                 tps;
  if (converted >= MIDI_SEQ_TIMING_RANGE) {
    converted = MIDI_SEQ_TIMING_RANGE - 1;
  }
  return (uint8_t)converted;
}

uint16_t MidiSeqTrack::timing_from_tick(uint32_t tick) const {
  uint16_t tps = ticks_per_step();
  if (tps == 0) return MIDI_SEQ_TIMING_CENTER;
  return (uint16_t)(tps + (tick % tps));
}

int32_t MidiSeqTrack::event_tick(uint8_t step,
                                 const MidiSeqEvent &event) const {
  return (int32_t)step * ticks_per_step() + data_timing_to_page(event.timing) -
         ticks_per_step();
}

void MidiSeqTrack::set_channel(uint8_t channel_) {
  seq_data.channel = channel_ & 0x0F;
}

uint8_t MidiSeqTrack::channel() const {
  return seq_data.channel;
}

void MidiSeqTrack::set_speed(uint8_t new_speed, bool) {
  speed = new_speed;
  seq_data.speed = new_speed;
  locks_slides_recalc = 255;
  for (uint8_t i = 0; i < MIDI_SEQ_NUM_LOCKS; i++) {
    locks_slide_data[i].init();
  }
}

void MidiSeqTrack::set_length(uint8_t len, bool) {
  if (len == 0) len = 1;
  if (len > MIDI_SEQ_NUM_STEPS) len = MIDI_SEQ_NUM_STEPS;
  length = len;
  seq_data.length = len;
  if (step_count >= length) {
    step_count %= length;
  }
  uint16_t start = 0, end = 0;
  seq_data.locate(step_count, start, end);
  cur_event_idx = start;
  locks_slides_recalc = 255;
  for (uint8_t i = 0; i < MIDI_SEQ_NUM_LOCKS; i++) {
    locks_slide_data[i].init();
  }

}

uint16_t MidiSeqTrack::add_event(uint8_t step, const MidiSeqEvent &event) {
  uint16_t idx = seq_data.add_event(step, event);
  if (idx != 0xFFFF) {
    if (step < step_count) cur_event_idx++;
    if (event.type == MIDI_SEQ_EVENT_LOCK) {
      invalidate_lock_slide(event.target, step);
    }
    epoch++;
  }
  return idx;
}

void MidiSeqTrack::remove_event(uint16_t index) {
  if (index >= seq_data.event_count) return;
  uint16_t start = 0;
  uint8_t step = 0;
  for (; step < MIDI_SEQ_NUM_STEPS; step++) {
    uint8_t bucket = seq_data.event_buckets[step];
    if (index < start + bucket) break;
    start += bucket;
  }
  MidiSeqEvent event = seq_data.events[index];
  seq_data.remove_event(index);
  if (step < step_count && cur_event_idx > 0) {
    cur_event_idx--;
  }
  if (event.type == MIDI_SEQ_EVENT_LOCK) {
    invalidate_lock_slide(event.target, step);
  }
  epoch++;
}

uint16_t MidiSeqTrack::find_note_event(uint8_t step, uint8_t note,
                                       bool note_on,
                                       uint16_t &start_idx) const {
  return seq_data.find_note_event(step, note, note_on, start_idx);
}

uint8_t MidiSeqTrack::search_note_off(uint8_t note, uint8_t step,
                                      uint16_t &event_idx,
                                      uint16_t event_end) const {
  return seq_data.search_note_off(note, step, event_idx, event_end, length);
}

bool MidiSeqTrack::add_note(uint32_t tick, uint32_t width, uint8_t note,
                            uint8_t velocity, uint8_t condition) {
  uint16_t tps = ticks_per_step();
  if (tps == 0 || length == 0) return false;

  uint32_t raw_step = tick / tps;
  uint16_t start_timing = (uint16_t)(tps + (tick % tps));
  uint32_t end_tick = tick + width;
  uint32_t raw_end_step = end_tick / tps;
  if (raw_end_step == raw_step) {
    raw_end_step++;
  }
  uint16_t end_timing =
      (uint16_t)(tps + end_tick - (raw_end_step * (uint32_t)tps));
  uint8_t step = (uint8_t)(raw_step % length);
  uint8_t end_step = (uint8_t)(raw_end_step % length);
  CLEAR_BIT128_P(oneshot_mask, step);

  uint16_t start_idx = 0;
  if (find_note_event(step, note, true, start_idx) != 0xFFFF) {
    return false;
  }
  start_idx = 0;
  if (find_note_event(end_step, note, false, start_idx) != 0xFFFF) {
    return false;
  }

  MidiSeqEvent on;
  on.init(MIDI_SEQ_EVENT_NOTE_ON, note, velocity, page_timing_to_data(start_timing),
          condition);
  MidiSeqEvent off;
  off.init(MIDI_SEQ_EVENT_NOTE_OFF, note, 0, page_timing_to_data(end_timing), 0);
  uint16_t on_idx = add_event(step, on);
  if (on_idx == 0xFFFF) {
    return false;
  }
  if (add_event(end_step, off) == 0xFFFF) {
    remove_event(on_idx);
    return false;
  }
  return true;
}

bool MidiSeqTrack::del_note(uint32_t tick, uint32_t width, uint8_t note) {
  uint16_t tps = ticks_per_step();
  bool note_on_found = false;
  bool ret = false;

  for (uint8_t step = 0; step < length; step++) {
  again:
    uint16_t start_idx = 0;
    uint16_t note_idx_on = find_note_event(step, note, true, start_idx);
    uint16_t ev_end = start_idx + seq_data.event_buckets[step];

    if (note_idx_on != 0xFFFF) {
      note_on_found = true;
      uint16_t note_idx_off = note_idx_on;
      uint8_t off_step = search_note_off(note, step, note_idx_off, ev_end);
      if (note_idx_off != 0xFFFF) {
        int32_t note_start = event_tick(step, seq_data.events[note_idx_on]);
        int32_t note_end = event_tick(off_step, seq_data.events[note_idx_off]);
        if (note_end < note_start) {
          note_end += length * tps;
        }
        if ((note_start <= tick + width) && (note_end > tick)) {
          remove_event(note_idx_off);
          start_idx = 0;
          note_idx_on = find_note_event(step, note, true, start_idx);
          remove_event(note_idx_on);
          note_off(note);
          ret = true;
          goto again;
        }
      }
    }

    if (note_on_found) continue;

    start_idx = 0;
    uint16_t note_idx_off = find_note_event(step, note, false, start_idx);
    if (note_idx_off != 0xFFFF) {
      int32_t note_end = event_tick(step, seq_data.events[note_idx_off]);
      if (note_end > tick) {
        remove_event(note_idx_off);
        for (uint8_t j = length - 1; j > step; j--) {
          start_idx = 0;
          note_idx_on = find_note_event(j, note, true, start_idx);
          if (note_idx_on != 0xFFFF) {
            remove_event(note_idx_on);
            break;
          }
        }
        note_off(note);
        ret = true;
        goto again;
      }
    }
  }
  return ret;
}

uint8_t MidiSeqTrack::selected_lock_param(uint8_t slot) const {
  if (slot >= MIDI_SEQ_NUM_LOCKS || !seq_data.locks[slot].is_active()) {
    return 0;
  }
  return seq_data.locks[slot].parameter + 1;
}

void MidiSeqTrack::set_selected_lock_param(uint8_t slot, uint8_t param) {
  if (slot >= MIDI_SEQ_NUM_LOCKS) return;
  if (param == 0) {
    seq_data.locks[slot].clear();
    invalidate_lock_slide(slot, 0);
    seq_data.clean_locks();
    epoch++;
    return;
  }
  uint16_t parameter = param - 1;
  seq_data.locks[slot].init(lock_type_for_legacy_param((uint8_t)parameter),
                            parameter);
  epoch++;
}

bool MidiSeqTrack::set_selected_lock_control(uint8_t slot, uint8_t type,
                                             uint16_t parameter,
                                             uint16_t default_value,
                                             uint8_t flags) {
  if (slot >= MIDI_SEQ_NUM_LOCKS) return false;
  if (type == MIDI_SEQ_LOCK_OFF) {
    seq_data.locks[slot].clear();
    invalidate_lock_slide(slot, 0);
    seq_data.clean_locks();
    epoch++;
    return true;
  }
  seq_data.locks[slot].init(type, parameter, default_value, flags);
  locks_slide_data[slot].init();
  locks_slides_recalc = 255;
  epoch++;
  return true;
}

bool MidiSeqTrack::add_lock(uint8_t step, uint16_t timing, uint8_t param,
                            uint8_t value, bool slide, uint8_t lock_idx) {
  if (step >= length || lock_idx >= MIDI_SEQ_NUM_LOCKS) {
    return false;
  }
  if (!seq_data.locks[lock_idx].is_active()) {
    seq_data.locks[lock_idx].init(lock_type_for_legacy_param(param), param);
  }
  MidiSeqEvent event;
  uint8_t flags = slide ? MIDI_SEQ_EVENT_FLAG_SLIDE : 0;
  event.init(MIDI_SEQ_EVENT_LOCK, lock_idx, value14_from_value7(value),
             page_timing_to_data(timing), 0, flags);
  return add_event(step, event) != 0xFFFF;
}

bool MidiSeqTrack::set_lock_event(uint8_t step, uint16_t timing, uint8_t type,
                                  uint16_t parameter, uint16_t value14,
                                  bool slide, uint16_t default_value,
                                  uint8_t flags) {
  if (step >= length) {
    return false;
  }

  uint8_t lock_idx = 255;
  for (uint8_t i = 0; i < MIDI_SEQ_NUM_LOCKS; i++) {
    const auto &lock = seq_data.locks[i];
    if (lock.is_active() && lock.type == type &&
        lock.parameter == parameter && lock.flags == flags) {
      lock_idx = i;
      break;
    }
  }
  if (lock_idx == 255) {
    for (uint8_t i = 0; i < MIDI_SEQ_NUM_LOCKS; i++) {
      if (!seq_data.locks[i].is_active()) {
        seq_data.locks[i].init(type, parameter, default_value, flags);
        seq_data.lock_count++;
        lock_idx = i;
        break;
      }
    }
  }
  if (lock_idx == 255) return false;

  uint8_t event_timing = page_timing_to_data(timing);
  uint16_t start = 0;
  uint16_t end = 0;
  seq_data.locate(step, start, end);
  for (uint16_t i = start; i < end;) {
    const auto &event = seq_data.events[i];
    if (event.type == MIDI_SEQ_EVENT_LOCK && event.target == lock_idx &&
        event.timing == event_timing) {
      remove_event(i);
      end--;
    } else {
      i++;
    }
  }

  MidiSeqEvent event;
  event.init(MIDI_SEQ_EVENT_LOCK, lock_idx, value14, event_timing, 0,
             slide ? MIDI_SEQ_EVENT_FLAG_SLIDE : 0);
  return add_event(step, event) != 0xFFFF;
}

bool MidiSeqTrack::del_lock(uint32_t tick, uint8_t lock_idx, uint8_t) {
  uint16_t tps = ticks_per_step();
  uint8_t step = tick / tps;
  if (step != 0) --step;
  uint16_t start = 0, end = 0;
  seq_data.locate(step, start, end);
  end = start;
  bool ret = false;
  constexpr uint8_t radius = 4;

  for (uint8_t n = step; n < min(length, (uint8_t)(step + 3)); n++) {
    end += seq_data.event_buckets[n];
    for (; start < end;) {
      uint16_t i = start;
      const auto &event = seq_data.events[i];
      if (event.type != MIDI_SEQ_EVENT_LOCK || event.target != lock_idx) {
        start++;
        continue;
      }
      int32_t target_x = (int32_t)tick;
      int32_t event_x = event_tick(n, event);
      if (event_x == target_x || (event_x <= target_x + radius &&
                                  event_x >= target_x - radius)) {
        remove_event(i);
        end--;
        ret = true;
      } else {
        start++;
      }
    }
  }
  return ret;
}

uint8_t MidiSeqTrack::count_lock_event(uint8_t step, uint8_t lock_idx) const {
  uint16_t start = 0, end = 0;
  seq_data.locate(step, start, end);
  uint8_t count = 0;
  for (uint16_t i = start; i < end; i++) {
    const auto &event = seq_data.events[i];
    if (event.type == MIDI_SEQ_EVENT_LOCK && event.target == lock_idx) count++;
  }
  return count;
}

uint8_t MidiSeqTrack::search_lock_idx(uint8_t lock_idx, uint8_t step,
                                      uint16_t &event_idx,
                                      uint16_t &event_end) const {
  uint8_t cur_step = step;
  event_idx++;
  do {
    for (; event_idx < event_end; event_idx++) {
      const auto &event = seq_data.events[event_idx];
      if (event.type == MIDI_SEQ_EVENT_LOCK && event.target == lock_idx) {
        return cur_step;
      }
    }
    cur_step++;
    if (cur_step >= length) {
      cur_step = 0;
      event_end = 0;
    }
    event_idx = event_end;
    event_end += seq_data.event_buckets[cur_step];
  } while (cur_step != step);
  event_idx = 0xFFFF;
  return step;
}

void MidiSeqTrack::clear_track_locks() {
  for (uint8_t i = 0; i < MIDI_SEQ_NUM_LOCKS; i++) {
    clear_track_locks(i);
  }
}

void MidiSeqTrack::clear_track_locks(uint8_t lock_idx) {
  for (uint16_t i = 0; i < seq_data.event_count;) {
    const auto &event = seq_data.events[i];
    if (event.type == MIDI_SEQ_EVENT_LOCK && event.target == lock_idx) {
      remove_event(i);
    } else {
      i++;
    }
  }
  if (lock_idx < MIDI_SEQ_NUM_LOCKS) {
    seq_data.locks[lock_idx].clear();
    locks_slide_data[lock_idx].init();
  }
  seq_data.clean_locks();
}

void MidiSeqTrack::init_notes_on() {
  notes_on_count = 0;
  for (uint8_t i = 0; i < NUM_NOTES_ON; i++) {
    notes_on[i].value = 255;
  }
}

bool MidiSeqTrack::note_on_at(uint8_t idx, NoteVector &note) const {
  if (idx >= NUM_NOTES_ON || notes_on[idx].value == 255) return false;
  note = notes_on[idx];
  return true;
}

void MidiSeqTrack::add_notes_on(uint8_t step, int16_t timing, uint8_t value,
                                uint8_t velocity) {
  uint8_t slot = 255;
  for (uint8_t i = 0; i < NUM_NOTES_ON; i++) {
    if (notes_on[i].value == value) {
      slot = i;
      break;
    }
    if (notes_on[i].value == 255 && slot == 255) slot = i;
  }
  if (slot == 255) return;
  if (notes_on[slot].value == 255) notes_on_count++;
  notes_on[slot].step = step;
  notes_on[slot].utiming = timing;
  notes_on[slot].value = value;
  notes_on[slot].velocity = velocity;
}

uint8_t MidiSeqTrack::find_notes_on(uint8_t value) const {
  for (uint8_t i = 0; i < NUM_NOTES_ON; i++) {
    if (notes_on[i].value == value) return i;
  }
  return 255;
}

void MidiSeqTrack::remove_notes_on(uint8_t value) {
  uint8_t idx = find_notes_on(value);
  if (idx == 255) return;
  notes_on[idx].value = 255;
  if (notes_on_count) notes_on_count--;
}

void MidiSeqTrack::note_on(uint8_t note, uint8_t velocity,
                           MidiUartClass *uart_) {
  if (uart_ == nullptr) uart_ = port_;
  if (uart_ == nullptr) uart_ = uart;
  if (!uart_) return;
  mixer_page.track_trig(DeviceIdx::Secondary, track_number, velocity);
  uart_->sendNoteOn(channel(), note, velocity);
  SET_BIT128_P(note_buffer, note);
}

void MidiSeqTrack::note_off(uint8_t note, uint8_t, MidiUartClass *uart_) {
  if (uart_ == nullptr) uart_ = port_;
  if (uart_ == nullptr) uart_ = uart;
  if (!uart_) return;
  uart_->sendNoteOff(channel(), note);
  CLEAR_BIT128_P(note_buffer, note);
}

void MidiSeqTrack::mute_on() {
  mute_state = SEQ_MUTE_ON;
  if (MidiClock.state == 2) {
    notesoff_pending = true;
  } else {
    buffer_notesoff();
  }
}

void MidiSeqTrack::toggle_mute() {
  if (mute_state == SEQ_MUTE_ON) {
    mute_state = SEQ_MUTE_OFF;
  } else {
    mute_on();
  }
}

void MidiSeqTrack::send_cc(uint8_t cc, uint8_t value, MidiUartClass *uart_) {
  if (uart_ == nullptr) uart_ = port_;
  if (uart_ == nullptr) uart_ = uart;
  if (!uart_) return;
  uart_->sendCC(channel(), cc, value);
}

void MidiSeqTrack::pitch_bend(uint16_t value, MidiUartClass *uart_) {
  if (uart_ == nullptr) uart_ = port_;
  if (uart_ == nullptr) uart_ = uart;
  if (!uart_) return;
  uart_->sendPitchBend(channel(), value);
}

void MidiSeqTrack::channel_pressure(uint8_t pressure, MidiUartClass *uart_) {
  if (uart_ == nullptr) uart_ = port_;
  if (uart_ == nullptr) uart_ = uart;
  if (!uart_) return;
  uart_->sendChannelPressure(channel(), pressure);
}

void MidiSeqTrack::after_touch(uint8_t note, uint8_t pressure,
                               MidiUartClass *uart_) {
  if (uart_ == nullptr) uart_ = port_;
  if (uart_ == nullptr) uart_ = uart;
  if (!uart_) return;
  uart_->sendPolyKeyPressure(channel(), note, pressure);
}

void MidiSeqTrack::buffer_notesoff() {
  if (!port_) port_ = uart;
  if (!port_) return;
  for (uint8_t note = 0; note < 128; note++) {
    if (IS_BIT_SET128_P(note_buffer, note)) {
      port_->sendNoteOff(channel(), note);
      CLEAR_BIT128_P(note_buffer, note);
    }
  }
  init_notes_on();
}

void MidiSeqTrack::record_track_noteon(uint8_t note, uint8_t velocity) {
  int16_t timing = tick_counter > 0 ? (int16_t)tick_counter - 1 : 0;
  uint8_t step = step_count;

  ignore_step = step;
  SET_BIT128_P(ignore_notes, note);
  add_notes_on(step, timing, note, velocity);
}

void MidiSeqTrack::record_track_noteoff(uint8_t note) {
  uint8_t idx = find_notes_on(note);
  if (idx == 255) return;

  if (MidiClock.state == 2 && SeqPage::recording) {
    uint16_t tps = ticks_per_step();
    int16_t timing = tick_counter > 0 ? (int16_t)tick_counter - 1 : 0;
    uint8_t step = step_count;
    uint32_t roll_length = (uint32_t)length * tps;
    uint32_t start_x = (uint32_t)notes_on[idx].step * tps +
                       (uint16_t)notes_on[idx].utiming;
    uint32_t end_x = (uint32_t)step * tps + (uint16_t)timing;

    ignore_step = step;
    SET_BIT128_P(ignore_notes, note);

    if (start_x >= roll_length) start_x %= roll_length;
    if (end_x >= roll_length) end_x %= roll_length;

    uint32_t width = 0;
    if (mcl_cfg.rec_quant) {
      if (end_x < start_x) end_x += roll_length;
      width = max((uint32_t)1, end_x - start_x);

      uint8_t q_step = notes_on[idx].step;
      if (notes_on[idx].utiming > (int16_t)(tps / 2)) {
        q_step++;
        if (q_step == length) q_step = 0;
      }
      start_x = (uint32_t)q_step * tps;
      end_x = start_x + width;
      if (end_x > roll_length) {
        del_note(0, end_x - roll_length, note);
      }
    } else {
      if (end_x < start_x) {
        del_note(0, end_x, note);
        end_x += roll_length;
      }
      width = end_x - start_x;
    }

    del_note(start_x, width, note);
    add_note(start_x, width, note, notes_on[idx].velocity, 0);
  }

  remove_notes_on(note);
}

bool MidiSeqTrack::record_lock(uint8_t type, uint16_t parameter,
                               uint16_t value, bool slide,
                               uint8_t lock_flags, uint16_t default_value) {
  if (!mcl_cfg.rec_automation || step_count >= length) {
    return false;
  }

  uint8_t lock_idx =
      seq_data.find_or_create_lock(type, parameter, default_value, lock_flags);
  if (lock_idx == 255) return false;

  uint16_t tps = ticks_per_step();
  int16_t timing = tick_counter > 0 ? (int16_t)tick_counter - 1 : 0;
  uint8_t step = step_count;
  uint16_t page_timing = tps + timing;
  if (mcl_cfg.rec_quant) {
    if (timing > (int16_t)(tps / 2)) {
      step++;
      if (step == length) step = 0;
    }
    page_timing = tps;
  }

  uint16_t start = 0, end = 0;
  seq_data.locate(step, start, end);
  for (uint16_t i = start; i < end;) {
    const auto &event = seq_data.events[i];
    if (event.type == MIDI_SEQ_EVENT_LOCK && event.target == lock_idx) {
      remove_event(i);
      end--;
    } else {
      i++;
    }
  }

  MidiSeqEvent event;
  event.init(MIDI_SEQ_EVENT_LOCK, lock_idx, value & 0x3FFF,
             page_timing_to_data(page_timing), 0,
             slide ? MIDI_SEQ_EVENT_FLAG_SLIDE : 0);
  return add_event(step, event) != 0xFFFF;
}

void MidiSeqTrack::reset_params() {
  if (!port_) port_ = uart;
  if (!port_) return;
  for (uint8_t i = 0; i < MIDI_SEQ_NUM_LOCKS; i++) {
    const auto &lock = seq_data.locks[i];
    if (!lock.is_active()) continue;
    MidiSeqEvent event;
    event.init(MIDI_SEQ_EVENT_LOCK, i, lock.default_value);
    send_lock_value(lock, event);
  }
}

void MidiSeqTrack::send_lock_value(const MidiSeqLockDefinition &lock,
                                   const MidiSeqEvent &event) {
  if (!port_ || !lock.is_active()) return;

  uint8_t value7 = value7_from_value14(event.value);
  switch (lock.type) {
  case MIDI_SEQ_LOCK_CC:
    port_->sendCC(channel(), (uint8_t)lock.parameter, value7);
    break;
  case MIDI_SEQ_LOCK_NRPN:
    port_->sendNRPN(channel(), lock.parameter, event.value);
    break;
  case MIDI_SEQ_LOCK_RPN:
    port_->sendRPN(channel(), lock.parameter, event.value);
    break;
  case MIDI_SEQ_LOCK_PITCH_BEND:
    port_->sendPitchBend(channel(), event.value);
    break;
  case MIDI_SEQ_LOCK_CHANNEL_PRESSURE:
    port_->sendChannelPressure(channel(), value7);
    break;
  case MIDI_SEQ_LOCK_PROGRAM_CHANGE:
    port_->sendProgramChange(channel(), value7);
    break;
  case MIDI_SEQ_LOCK_POLY_PRESSURE:
    port_->sendPolyKeyPressure(channel(), (uint8_t)lock.parameter, value7);
    break;
  default:
    break;
  }
}

void MidiSeqTrack::invalidate_lock_slide(uint8_t lock_idx, uint8_t) {
  if (lock_idx >= MIDI_SEQ_NUM_LOCKS) return;
  locks_slide_data[lock_idx].init();
}

void MidiSeqTrack::prepare_slide(uint8_t lock_idx, int32_t x0, int32_t x1,
                                 uint16_t y0, uint16_t y1) {
  if (lock_idx >= MIDI_SEQ_NUM_LOCKS) return;
  auto &slide = locks_slide_data[lock_idx];
  slide.x0 = x0;
  slide.x1 = x1;
  slide.y0 = y0;
  slide.y1 = y1;
  slide.accum = 0;

  int32_t steps = x1 - x0;
  slide.delta = steps > 0 ? ((((int32_t)y1 - (int32_t)y0) << 8) / steps) : 0;
}

void MidiSeqTrack::send_slides() {
  for (uint8_t i = 0; i < MIDI_SEQ_NUM_LOCKS; i++) {
    const auto &lock = seq_data.locks[i];
    if (!lock.is_active()) continue;

    auto &slide = locks_slide_data[i];
    if (slide.x0 >= slide.x1) continue;

    int32_t value = (int32_t)slide.y0 + (slide.accum >> 8);
    slide.accum += slide.delta;
    slide.x0++;

    if (slide.x0 >= slide.x1) {
      value = slide.y1;
      slide.init();
    }

    MidiSeqEvent event;
    event.init(MIDI_SEQ_EVENT_LOCK, i, clamp_value14(value));
    send_lock_value(lock, event);
  }
}

void MidiSeqTrack::find_next_locks(uint16_t curidx, uint8_t step,
                                   bool *find_array) {
  uint8_t count = 0;
  for (uint8_t i = 0; i < MIDI_SEQ_NUM_LOCKS; i++) {
    if (find_array[i]) count++;
  }
  if (count == 0 || length == 0) return;

  curidx += seq_data.event_buckets[step];
  uint8_t next_step = step + 1;
  uint16_t end = curidx;
  uint8_t max_len = length;

again:
  for (; next_step < max_len; next_step++) {
    end += seq_data.event_buckets[next_step];
    for (; curidx < end; curidx++) {
      if (count == 0) return;
      const auto &event = seq_data.events[curidx];
      if (event.type != MIDI_SEQ_EVENT_LOCK ||
          event.target >= MIDI_SEQ_NUM_LOCKS ||
          !seq_data.locks[event.target].is_active()) {
        continue;
      }

      uint8_t lock_idx = event.target;
      if (!find_array[lock_idx]) continue;

      locks_slide_next_lock_val[lock_idx] = event.value;
      locks_slide_next_lock_step[lock_idx] = next_step;
      locks_slide_next_lock_timing[lock_idx] = event.timing;
      find_array[lock_idx] = false;
      count--;
    }
  }

  if (next_step == length) {
    next_step = 0;
    curidx = 0;
    end = 0;
    max_len = step;
    goto again;
  }
}

void MidiSeqTrack::recalc_slides() {
  if (locks_slides_recalc == 255) return;

  uint8_t step = locks_slides_recalc;
  if (step >= length || step >= MIDI_SEQ_NUM_STEPS || length == 0) {
    locks_slides_recalc = 255;
    return;
  }

  uint16_t bucket_start = 0;
  uint16_t bucket_end = 0;
  seq_data.locate(step, bucket_start, bucket_end);
  uint16_t curidx = locks_slides_idx;
  if (curidx < bucket_start || curidx > bucket_end) {
    curidx = bucket_start;
  }

  bool find_array[MIDI_SEQ_NUM_LOCKS] = {};
  bool find = false;
  uint8_t bucket = seq_data.event_buckets[step];
  for (int16_t n = (int16_t)bucket - 1; n >= 0; n--) {
    const auto &event = seq_data.events[curidx + n];
    if (event.type == MIDI_SEQ_EVENT_LOCK &&
        (event.flags() & MIDI_SEQ_EVENT_FLAG_SLIDE) &&
        event.target < MIDI_SEQ_NUM_LOCKS &&
        seq_data.locks[event.target].is_active()) {
      find_array[event.target] = true;
      find = true;
    }
  }

  if (!find) {
    locks_slides_recalc = 255;
    return;
  }

  find_next_locks(curidx, step, find_array);

  uint16_t tps = ticks_per_step();
  for (int16_t n = (int16_t)bucket - 1; n >= 0; n--) {
    const auto &event = seq_data.events[curidx + n];
    if (event.type != MIDI_SEQ_EVENT_LOCK ||
        !(event.flags() & MIDI_SEQ_EVENT_FLAG_SLIDE) ||
        event.target >= MIDI_SEQ_NUM_LOCKS ||
        !seq_data.locks[event.target].is_active()) {
      continue;
    }

    uint8_t lock_idx = event.target;
    if (find_array[lock_idx]) {
      locks_slide_data[lock_idx].init();
      continue;
    }

    uint8_t next_step = locks_slide_next_lock_step[lock_idx];
    if (step == next_step) {
      locks_slide_data[lock_idx].init();
      continue;
    }

    int32_t x0 = (int32_t)step * tps + data_timing_to_page(event.timing) -
                 tps + 1;
    uint16_t next_timing =
        data_timing_to_page(locks_slide_next_lock_timing[lock_idx]);
    int32_t x1 = next_step < step
                     ? (int32_t)(length + next_step) * tps + next_timing -
                           tps - 1
                     : (int32_t)next_step * tps + next_timing - tps - 1;

    prepare_slide(lock_idx, x0, x1, event.value,
                  locks_slide_next_lock_val[lock_idx]);
  }

  locks_slides_recalc = 255;
}

void MidiSeqTrack::handle_event(const MidiSeqEvent &event, uint8_t step,
                                uint16_t bucket_start) {
  if (event.type == MIDI_SEQ_EVENT_NOTE_ON) {
    if (step == ignore_step && IS_BIT_SET128_P(ignore_notes, event.target)) {
      return;
    }
    if (IS_BIT_SET128_P(mute_mask, step)) {
      return;
    }
    if (conditional_for_event(event.condition, step)) {
      note_on(event.target, (uint8_t)event.value);
    }
  } else if (event.type == MIDI_SEQ_EVENT_NOTE_OFF) {
    if (step == ignore_step && IS_BIT_SET128_P(ignore_notes, event.target)) {
      return;
    }
    note_off(event.target);
  } else if (event.type == MIDI_SEQ_EVENT_LOCK &&
             event.target < MIDI_SEQ_NUM_LOCKS) {
    if (conditional_for_event(event.condition, step)) {
      send_lock_value(seq_data.locks[event.target], event);
      if (event.flags() & MIDI_SEQ_EVENT_FLAG_SLIDE) {
        locks_slides_recalc = step;
        locks_slides_idx = bucket_start;
      }
    }
  }
}

bool MidiSeqTrack::conditional_for_event(uint8_t condition, uint8_t step) {
  if (condition == MIDI_SEQ_COND_ONESHOT) {
    if (IS_BIT_SET128_P(oneshot_mask, step)) {
      return false;
    }
    SET_BIT128_P(oneshot_mask, step);
    return true;
  }
  return conditional(condition);
}

void MidiSeqTrack::seq(MidiUartClass *uart_) {
  port_ = uart_;
  if (tick_counter == 0) {
    uint8_t pending_speed;
    if (consume_pending_speed_change(pending_speed)) {
      set_speed(pending_speed, true);
    }
  }

  uint16_t tps = ticks_per_step();
  tick_counter++;
  update_legacy_progress_counter();

  if (tick_counter >= tps) {
    cur_event_idx += seq_data.event_buckets[step_count];
    tick_counter = 0;
    mod12_counter = 0;
    if (ignore_step == step_count) {
      ignore_step = 255;
      memset(ignore_notes, 0, sizeof(ignore_notes));
    }
    step_count_inc();
  }

  if (count_down) {
    count_down--;
    if (count_down == 0) {
      if (!cache_loaded) {
        load_cache();
        cache_loaded = true;
      }
      reset();
    } else if (!cache_loaded &&
               SeqTrackTransition::in_cache_window(
                   SEQ_TRANSITION_CACHE_MIDI_LINEAR, count_down,
                   track_number)) {
      load_cache();
      cache_loaded = true;
    }
    return;
  }

  if (notesoff_pending) {
    notesoff_pending = false;
    buffer_notesoff();
  }

  if (record_mutes) {
    uint8_t timing = 0;
    uint8_t quant = 0;
    uint8_t step = get_quantized_step(timing, quant);
    SET_BIT128_P(mute_mask, step);
  }

  if (mute_state == SEQ_MUTE_ON) {
    return;
  }

  send_slides();

  uint16_t ev_idx = cur_event_idx;
  uint16_t ev_end = cur_event_idx + seq_data.event_buckets[step_count];
  uint16_t bucket_start = cur_event_idx;
  for (; ev_idx < ev_end; ev_idx++) {
    const auto &event = seq_data.events[ev_idx];
    uint16_t timing = data_timing_to_page(event.timing);
    if (current_event_due(timing, tps, tick_counter)) {
      handle_event(event, step_count, bucket_start);
    }
  }

  uint8_t next_step = step_count == length - 1 ? 0 : step_count + 1;
  ev_idx = next_step == 0 ? 0 : ev_end;
  ev_end = ev_idx + seq_data.event_buckets[next_step];
  bucket_start = ev_idx;
  for (; ev_idx < ev_end; ev_idx++) {
    const auto &event = seq_data.events[ev_idx];
    uint16_t timing = data_timing_to_page(event.timing);
    if (next_event_due(timing, tps, tick_counter)) {
      handle_event(event, next_step, bucket_start);
    }
  }
}

void MidiSeqTrack::clear_track(bool) {
  uint8_t ch = seq_data.channel;
  uint8_t len = length ? length : 16;
  uint8_t spd = speed;
  seq_data.clear();
  seq_data.channel = ch;
  seq_data.length = len;
  seq_data.speed = spd;
  memset(oneshot_mask, 0, sizeof(oneshot_mask));
  memset(ignore_notes, 0, sizeof(ignore_notes));
  memset(mute_mask, 0, sizeof(mute_mask));
  length = len;
  speed = spd;
  tick_counter = 0;
  mod12_counter = 0;
  locks_slides_recalc = 255;
  locks_slides_idx = 0;
  for (uint8_t i = 0; i < MIDI_SEQ_NUM_LOCKS; i++) {
    locks_slide_data[i].init();
  }
  notesoff_pending = true;
  epoch++;
}

void MidiSeqTrack::clear_mute() {
  memset(mute_mask, 0, sizeof(mute_mask));
}

void MidiSeqTrack::transpose(int8_t offset) {
  for (uint16_t i = 0; i < seq_data.event_count; i++) {
    auto &event = seq_data.events[i];
    if (event.type == MIDI_SEQ_EVENT_NOTE_ON ||
        event.type == MIDI_SEQ_EVENT_NOTE_OFF) {
      int16_t note = event.target + offset;
      if (note < 0) note = 0;
      if (note > 127) note = 127;
      event.target = (uint8_t)note;
    }
  }
  epoch++;
}

void MidiSeqTrack::import_legacy_ext(const ExtSeqTrackData &legacy,
                                     const GridLink &link) {
  seq_data.clear();
  seq_data.channel = legacy.channel;
  seq_data.length = link.length ? link.length : 16;
  seq_data.speed = link.speed_value();
  length = seq_data.length;
  speed = seq_data.speed;
  locks_slides_recalc = 255;
  locks_slides_idx = 0;
  for (uint8_t i = 0; i < MIDI_SEQ_NUM_LOCKS; i++) {
    locks_slide_data[i].init();
  }

  const uint8_t legacy_tps = legacy_ticks_for_speed(link.speed_value());
  uint16_t idx = 0;
  for (uint8_t step = 0; step < NUM_EXT_STEPS; step++) {
    uint8_t bucket = const_cast<ExtSeqTrackData &>(legacy).event_buckets.get(step);
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
        seq_data.locks[lock_idx].init(lock_type_for_legacy_param(param), param,
                                      0, 0);
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
  seq_data.clean_locks();
  epoch++;
}

#endif // !defined(__AVR__)
