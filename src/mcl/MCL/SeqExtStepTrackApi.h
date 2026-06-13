/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQEXTSTEPTRACKAPI_H__
#define SEQEXTSTEPTRACKAPI_H__

#if !defined(__AVR__)
#define SEQ_EXTSTEP_HAS_MIDI_TRACK 1
#endif

#include "ExtSeqTrack.h"
#include "SeqExtStepLockApi.h"
#ifdef PLATFORM_TBD
#include "SeqExtMidiControl.h"
#endif
#include "SeqExtStepTypes.h"
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
#include "../Drivers/Generic/Sequencer/MidiSeqTrack.h"
#endif
#include <stdint.h>
#include <stddef.h>

struct SeqExtStepEvent {
  bool is_lock;
  bool event_on;
  uint8_t lock_idx;
  uint8_t cond_id;
  uint16_t event_value;
  int16_t micro_timing;
};

class SeqExtStepTrackApi {
public:
  explicit SeqExtStepTrackApi(ExtSeqTrack &track) : ext_track_(&track) {}
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
  explicit SeqExtStepTrackApi(MidiSeqTrack &track) : midi_track_(&track) {}
#endif

  SeqExtStepLockApi locks() const {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) return SeqExtStepLockApi(*midi_track_);
#endif
    return SeqExtStepLockApi(*ext_track_);
  }

#ifdef PLATFORM_TBD
  bool parse_control_change(SeqExtMidiControlState &control_state,
                            uint8_t channel, uint8_t cc, uint8_t value,
                            SeqExtParsedControl &out) const {
    return midi_track_ != nullptr &&
           control_state.parse_cc(channel, cc, value, out);
  }
#endif

  uint8_t length() const {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) return midi_track_->length;
#endif
    return ext_track_->length;
  }

  uint8_t speed() const {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) return midi_track_->speed;
#endif
    return ext_track_->speed;
  }

  void set_length(uint8_t len, bool expand = false) {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) {
      midi_track_->set_length(len, expand);
      return;
    }
#endif
    ext_track_->set_length(len, expand);
  }

  void set_speed(uint8_t new_speed) {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) {
      midi_track_->set_speed(new_speed);
      return;
    }
#endif
    ext_track_->set_speed(new_speed);
  }

  uint8_t step_count() const {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) return midi_track_->step_count;
#endif
    return ext_track_->step_count;
  }

  uint16_t mod_ticks() const {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) return midi_track_->tick_counter;
#endif
    return ext_track_->mod12_counter;
  }

  uint16_t ticks_per_step() const {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) return midi_track_->ticks_per_step();
#endif
    return ext_track_->get_ticks_per_step();
  }

  uint16_t speed_multiplier_int() const {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) return midi_track_->speed_multiplier_int();
#endif
    return ext_track_->get_speed_multiplier_int();
  }

  seq_extstep_tick_t step_tick(uint8_t step) const {
    return (seq_extstep_tick_t)step * ticks_per_step();
  }

  seq_extstep_tick_t event_tick(uint8_t step,
                                const SeqExtStepEvent &event) const {
    return (seq_extstep_tick_t)step * ticks_per_step() + event.micro_timing -
           ticks_per_step();
  }

  uint8_t step_from_tick(seq_extstep_tick_t tick) const {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) return midi_track_->step_from_tick(tick);
#endif
    return tick / ticks_per_step();
  }

  uint16_t timing_from_tick(seq_extstep_tick_t tick) const {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) return midi_track_->timing_from_tick(tick);
#endif
    uint8_t step = step_from_tick(tick);
    return ticks_per_step() + tick - ((uint16_t)step * ticks_per_step());
  }

  uint8_t event_bucket_size(uint8_t step) const {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) return midi_track_->seq_data.event_buckets[step];
#endif
    return ext_track_->event_buckets.get(step);
  }

  SeqExtStepEvent event(uint16_t idx) const {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) {
      const auto &event = midi_track_->seq_data.events[idx];
      return {event.type == MIDI_SEQ_EVENT_LOCK,
              event.type == MIDI_SEQ_EVENT_LOCK
                  ? (event.flags() & MIDI_SEQ_EVENT_FLAG_SLIDE) != 0
                  : event.type == MIDI_SEQ_EVENT_NOTE_ON,
              event.target,
              event.condition,
              event.type == MIDI_SEQ_EVENT_LOCK ? value7_from_14(event.value)
                                                : event.target,
              (int16_t)midi_track_->data_timing_to_page(event.timing)};
    }
#endif
    const auto &event = ext_track_->events[idx];
    return {event.is_lock, event.event_on, event.lock_idx,
            ext_event_condition(event),
            event.event_value,
            (int16_t)SeqTrack::microtiming_to_timing(event.micro_timing,
                                                     ticks_per_step())};
  }

  uint8_t note_velocity(uint8_t step, uint16_t event_idx) const {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) {
      const auto &event = midi_track_->seq_data.events[event_idx];
      return event.type == MIDI_SEQ_EVENT_NOTE_ON ? (uint8_t)event.value : 0;
    }
#endif
    return ext_track_->velocities[step];
  }

  uint8_t search_note_off(uint8_t note, uint8_t step, uint16_t &event_idx,
                          uint16_t event_end) {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) {
      return midi_track_->search_note_off(note, step, event_idx, event_end);
    }
#endif
    return ext_track_->search_note_off(note, step, event_idx, event_end);
  }

  uint8_t search_lock(uint8_t lock_idx, uint8_t step, uint16_t &event_idx,
                      uint16_t &event_end) {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) {
      return midi_track_->search_lock_idx(lock_idx, step, event_idx, event_end);
    }
#endif
    return ext_track_->search_lock_idx(lock_idx, step, event_idx, event_end);
  }

  bool delete_note(seq_extstep_tick_t tick, seq_extstep_tick_t width,
                   uint8_t note) {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) return midi_track_->del_note(tick, width, note);
#endif
    return ext_track_->del_note((uint16_t)tick, (uint16_t)width, note);
  }

  bool delete_notes(seq_extstep_tick_t tick, seq_extstep_tick_t width,
                    uint8_t note_min = 0, uint8_t note_max = 127) {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) {
      bool changed = false;
      for (uint8_t note = note_min; note <= note_max; note++) {
        changed |= midi_track_->del_note(tick, width, note);
        if (note == note_max) break;
      }
      return changed;
    }
#endif
    return ext_track_->del_notes((uint16_t)tick, (uint16_t)width, note_min,
                                 note_max);
  }

  void add_note(seq_extstep_tick_t tick, seq_extstep_tick_t width,
                uint8_t note, uint8_t velocity, uint8_t condition) {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) {
      midi_track_->add_note(tick, width, note, velocity, condition);
      return;
    }
#endif
    ext_track_->add_note((uint16_t)tick, (uint16_t)width, note, velocity,
                         condition);
  }

  void replace_note(seq_extstep_tick_t tick, seq_extstep_tick_t width,
                    uint8_t note, uint8_t velocity, uint8_t condition) {
    if (width <= 0) width = 1;
    delete_note(tick, width - 1, note);
    add_note(tick, width, note, velocity, condition);
  }

  bool toggle_note(seq_extstep_tick_t tick, seq_extstep_tick_t width,
                   uint8_t note, uint8_t velocity, uint8_t condition) {
    if (width <= 0) width = 1;
    if (delete_note(tick, width - 1, note)) {
      return true;
    }
    add_note(tick, width, note, velocity, condition);
    return true;
  }

  uint8_t notes_on_count() const {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) return midi_track_->notes_on_count;
#endif
    return ext_track_->notes_on_count;
  }

  bool note_on_at(uint8_t idx, NoteVector &note) const {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) return midi_track_->note_on_at(idx, note);
#endif
    if (idx >= NUM_NOTES_ON || ext_track_->notes_on[idx].value == 255) return false;
    note = ext_track_->notes_on[idx];
    return true;
  }

  uint8_t change_counter() const {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) return MidiSeqTrack::epoch;
#endif
    return ExtSeqTrack::epoch;
  }

  static uint8_t global_change_counter() {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    return MidiSeqTrack::epoch;
#else
    return ExtSeqTrack::epoch;
#endif
  }

  uint8_t channel() const {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) return midi_track_->channel();
#endif
    return ext_track_->channel;
  }

  void set_channel(uint8_t channel) {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) {
      if (midi_track_->channel() != channel) {
        midi_track_->buffer_notesoff();
        midi_track_->set_channel(channel);
      }
      return;
    }
#endif
    if (ext_track_->channel != channel) {
      ext_track_->buffer_notesoff();
      ext_track_->channel = channel;
    }
  }

  void clear_track() {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) {
      midi_track_->clear_track();
      return;
    }
#endif
    ext_track_->clear_track();
  }

  void clear_track_locks() {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) {
      midi_track_->clear_track_locks();
      return;
    }
#endif
    ext_track_->clear_track_locks();
  }

  void clear_track_locks(uint8_t lock_idx) {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) {
      midi_track_->clear_track_locks(lock_idx);
      return;
    }
#endif
    ext_track_->clear_track_locks(lock_idx);
  }

  void note_on(uint8_t note, uint8_t velocity,
               MidiUartClass *uart_ = nullptr) {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) {
      midi_track_->note_on(note, velocity, uart_);
      return;
    }
#endif
    ext_track_->note_on(note, velocity, uart_);
  }

  void note_off(uint8_t note, uint8_t velocity = 0,
                MidiUartClass *uart_ = nullptr) {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) {
      midi_track_->note_off(note, velocity, uart_);
      return;
    }
#endif
    ext_track_->note_off(note, velocity, uart_);
  }

  void send_cc(uint8_t cc, uint8_t value, MidiUartClass *uart_ = nullptr) {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) {
      midi_track_->send_cc(cc, value, uart_);
      return;
    }
#endif
    ext_track_->send_cc(cc, value, uart_);
  }

  void pitch_bend(uint16_t value, MidiUartClass *uart_ = nullptr) {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) {
      midi_track_->pitch_bend(value, uart_);
      return;
    }
#endif
    ext_track_->pitch_bend(value, uart_);
  }

  void channel_pressure(uint8_t pressure, MidiUartClass *uart_ = nullptr) {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) {
      midi_track_->channel_pressure(pressure, uart_);
      return;
    }
#endif
    ext_track_->channel_pressure(pressure, uart_);
  }

  void after_touch(uint8_t note, uint8_t pressure,
                   MidiUartClass *uart_ = nullptr) {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) {
      midi_track_->after_touch(note, pressure, uart_);
      return;
    }
#endif
    ext_track_->after_touch(note, pressure, uart_);
  }

  void buffer_notesoff() {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) {
      midi_track_->buffer_notesoff();
      return;
    }
#endif
    ext_track_->buffer_notesoff();
  }

  void init_notes_on() {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) {
      midi_track_->init_notes_on();
      return;
    }
#endif
    ext_track_->init_notes_on();
  }

  void update_param(uint8_t param_id, uint8_t value) {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) {
      (void)param_id;
      (void)value;
      return;
    }
#endif
    ext_track_->update_param(param_id, value);
  }

  void record_note_on(uint8_t note, uint8_t velocity) {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) {
      midi_track_->record_track_noteon(note, velocity);
      return;
    }
#endif
    ext_track_->record_track_noteon(note, velocity);
  }

  void record_note_off(uint8_t note) {
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
    if (midi_track_) {
      midi_track_->record_track_noteoff(note);
      return;
    }
#endif
    ext_track_->record_track_noteoff(note);
  }

  // Lock-recording helpers. AVR forwards directly to ExtSeqTrack's
  // 7-bit lock store; non-AVR routes through SeqExtStepLockApi's
  // generalized 14-bit record_control_lock which itself dispatches
  // between ExtSeqTrack and MidiSeqTrack at runtime.
  void record_cc_lock(uint8_t param, uint8_t value, bool slide) {
#if defined(__AVR__)
    ext_track_->record_track_locks(param, value, slide);
#else
    locks().record_control_lock(SEQ_EXT_LOCK_CTRL_CC, param, value, slide);
#endif
  }

  void record_pitch_bend_lock(uint16_t value14, bool slide) {
#if defined(__AVR__)
    if (value14 > 0x3FFF) value14 = 0x3FFF;
    ext_track_->record_track_locks(PARAM_PB, (uint8_t)(value14 >> 7), slide);
#else
    locks().record_control_lock(SEQ_EXT_LOCK_CTRL_PITCH_BEND, 0, value14,
                                slide);
#endif
  }

  void record_channel_pressure_lock(uint8_t value) {
#if defined(__AVR__)
    ext_track_->record_track_locks(PARAM_CHP, value, false);
#else
    locks().record_control_lock(SEQ_EXT_LOCK_CTRL_CHANNEL_PRESSURE, 0, value,
                                false);
#endif
  }

private:
  static uint8_t value7_from_14(uint16_t value14) {
    if (value14 > 0x3FFF) value14 = 0x3FFF;
    return (uint8_t)(((uint32_t)value14 * 127u + 0x1FFFu) / 0x3FFFu);
  }

  ExtSeqTrack *ext_track_ = nullptr;
#ifdef SEQ_EXTSTEP_HAS_MIDI_TRACK
  MidiSeqTrack *midi_track_ = nullptr;
#endif
};

#undef SEQ_EXTSTEP_HAS_MIDI_TRACK

#endif /* SEQEXTSTEPTRACKAPI_H__ */
