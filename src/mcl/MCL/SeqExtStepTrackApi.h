/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQEXTSTEPTRACKAPI_H__
#define SEQEXTSTEPTRACKAPI_H__

#include "ExtSeqTrack.h"
#ifdef PLATFORM_TBD
#include "../Drivers/Generic/Sequencer/MidiSeqTrack.h"
#endif
#include <stdint.h>

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
#ifdef PLATFORM_TBD
  explicit SeqExtStepTrackApi(MidiSeqTrack &track) : midi_track_(&track) {}
#endif

  uint8_t length() const {
#ifdef PLATFORM_TBD
    if (midi_track_) return midi_track_->length;
#endif
    return ext_track_->length;
  }

  uint8_t speed() const {
#ifdef PLATFORM_TBD
    if (midi_track_) return midi_track_->speed;
#endif
    return ext_track_->speed;
  }

  uint8_t step_count() const {
#ifdef PLATFORM_TBD
    if (midi_track_) return midi_track_->step_count;
#endif
    return ext_track_->step_count;
  }

  uint16_t mod_ticks() const {
#ifdef PLATFORM_TBD
    if (midi_track_) return midi_track_->tick_counter;
#endif
    return ext_track_->mod12_counter;
  }

  uint16_t ticks_per_step() const {
#ifdef PLATFORM_TBD
    if (midi_track_) return midi_track_->ticks_per_step();
#endif
    return ext_track_->get_timing_mid();
  }

  uint16_t speed_multiplier_int() const {
#ifdef PLATFORM_TBD
    if (midi_track_) return midi_track_->speed_multiplier_int();
#endif
    return ext_track_->get_speed_multiplier_int();
  }

  uint16_t step_tick(uint8_t step) const {
    return (uint16_t)step * ticks_per_step();
  }

  int16_t event_tick(uint8_t step, const SeqExtStepEvent &event) const {
    return (int16_t)step * ticks_per_step() + event.micro_timing -
           ticks_per_step();
  }

  uint8_t step_from_tick(uint16_t tick) const {
#ifdef PLATFORM_TBD
    if (midi_track_) return midi_track_->step_from_tick(tick);
#endif
    return tick / ticks_per_step();
  }

  uint16_t timing_from_tick(uint16_t tick) const {
#ifdef PLATFORM_TBD
    if (midi_track_) return midi_track_->timing_from_tick(tick);
#endif
    uint8_t step = step_from_tick(tick);
    return ticks_per_step() + tick - ((uint16_t)step * ticks_per_step());
  }

  uint8_t event_bucket_size(uint8_t step) const {
#ifdef PLATFORM_TBD
    if (midi_track_) return midi_track_->seq_data.event_buckets[step];
#endif
    return ext_track_->event_buckets.get(step);
  }

  SeqExtStepEvent event(uint16_t idx) const {
#ifdef PLATFORM_TBD
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
    return {event.is_lock, event.event_on, event.lock_idx, event.cond_id,
            event.event_value, event.micro_timing};
  }

  uint8_t search_note_off(uint8_t note, uint8_t step, uint16_t &event_idx,
                          uint16_t event_end) {
#ifdef PLATFORM_TBD
    if (midi_track_) {
      return midi_track_->search_note_off(note, step, event_idx, event_end);
    }
#endif
    return ext_track_->search_note_off(note, step, event_idx, event_end);
  }

  uint8_t search_lock(uint8_t lock_idx, uint8_t step, uint16_t &event_idx,
                      uint16_t &event_end) {
#ifdef PLATFORM_TBD
    if (midi_track_) {
      return midi_track_->search_lock_idx(lock_idx, step, event_idx, event_end);
    }
#endif
    return ext_track_->search_lock_idx(lock_idx, step, event_idx, event_end);
  }

  bool delete_note(uint16_t tick, uint16_t width, uint8_t note) {
#ifdef PLATFORM_TBD
    if (midi_track_) return midi_track_->del_note(tick, width, note);
#endif
    return ext_track_->del_note(tick, width, note);
  }

  void add_note(uint16_t tick, uint16_t width, uint8_t note, uint8_t velocity,
                uint8_t condition) {
#ifdef PLATFORM_TBD
    if (midi_track_) {
      midi_track_->add_note(tick, width, note, velocity, condition);
      return;
    }
#endif
    ext_track_->add_note(tick, width, note, velocity, condition);
  }

  bool delete_lock(uint16_t tick, uint8_t lock_idx, uint8_t value) {
#ifdef PLATFORM_TBD
    if (midi_track_) return midi_track_->del_lock(tick, lock_idx, value);
#endif
    return ext_track_->del_track_locks(tick, lock_idx, value);
  }

  bool add_lock(uint8_t step, uint16_t timing, uint8_t param, uint8_t value,
                bool slide, uint8_t lock_idx) {
#ifdef PLATFORM_TBD
    if (midi_track_) {
      return midi_track_->add_lock(step, timing, param, value, slide, lock_idx);
    }
#endif
    return ext_track_->set_track_locks(step, timing, param, value, slide,
                                       lock_idx);
  }

  uint8_t count_lock_event(uint8_t step, uint8_t lock_idx) {
#ifdef PLATFORM_TBD
    if (midi_track_) return midi_track_->count_lock_event(step, lock_idx);
#endif
    return ext_track_->count_lock_event(step, lock_idx);
  }

  uint8_t selected_lock_param(uint8_t slot) const {
#ifdef PLATFORM_TBD
    if (midi_track_) return midi_track_->selected_lock_param(slot);
#endif
    return ext_track_->locks_params[slot];
  }

  void set_selected_lock_param(uint8_t slot, uint8_t param) {
#ifdef PLATFORM_TBD
    if (midi_track_) {
      midi_track_->set_selected_lock_param(slot, param);
      return;
    }
#endif
    ext_track_->locks_params[slot] = param;
  }

  uint8_t notes_on_count() const {
#ifdef PLATFORM_TBD
    if (midi_track_) return midi_track_->notes_on_count;
#endif
    return ext_track_->notes_on_count;
  }

  bool note_on_at(uint8_t idx, NoteVector &note) const {
#ifdef PLATFORM_TBD
    if (midi_track_) return midi_track_->note_on_at(idx, note);
#endif
    if (idx >= NUM_NOTES_ON || ext_track_->notes_on[idx].value == 255) return false;
    note = ext_track_->notes_on[idx];
    return true;
  }

  uint8_t change_counter() const {
#ifdef PLATFORM_TBD
    if (midi_track_) return MidiSeqTrack::epoch;
#endif
    return ExtSeqTrack::epoch;
  }

  static uint8_t global_change_counter() {
#ifdef PLATFORM_TBD
    return MidiSeqTrack::epoch;
#else
    return ExtSeqTrack::epoch;
#endif
  }

private:
  static uint8_t value7_from_14(uint16_t value14) {
    if (value14 > 0x3FFF) value14 = 0x3FFF;
    return (uint8_t)(((uint32_t)value14 * 127u + 0x1FFFu) / 0x3FFFu);
  }

  ExtSeqTrack *ext_track_ = nullptr;
#ifdef PLATFORM_TBD
  MidiSeqTrack *midi_track_ = nullptr;
#endif
};

#endif /* SEQEXTSTEPTRACKAPI_H__ */
