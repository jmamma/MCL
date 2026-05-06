/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQEXTSTEPTRACKAPI_H__
#define SEQEXTSTEPTRACKAPI_H__

#include "ExtSeqTrack.h"
#ifdef PLATFORM_TBD
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

struct SeqExtStepLockParamInfo {
  bool active = false;
  bool p4_param = false;
  bool sendable = false;
  bool nrpn = false;
  bool macro = false;
  uint8_t type = 0;
  uint8_t ctrl = 0;
  uint8_t ctrl_type = 0;
  uint16_t param_id = 0;
  uint16_t resolution = 128;
  int16_t min_value = 0;
  int16_t max_value = 127;
  int16_t default_value = 0;
  int16_t current_value = 0;
};

enum SeqExtStepLockCtrlType : uint8_t {
  SEQ_EXT_LOCK_CTRL_OFF = 0,
  SEQ_EXT_LOCK_CTRL_CC = 1,
  SEQ_EXT_LOCK_CTRL_NRPN = 2,
  SEQ_EXT_LOCK_CTRL_RPN = 3,
  SEQ_EXT_LOCK_CTRL_PITCH_BEND = 4,
  SEQ_EXT_LOCK_CTRL_CHANNEL_PRESSURE = 5,
  SEQ_EXT_LOCK_CTRL_PROGRAM_CHANGE = 6,
  SEQ_EXT_LOCK_CTRL_POLY_PRESSURE = 7,
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

  bool selected_lock_param_id(uint8_t slot, uint8_t &param_id) const {
    uint8_t selected = selected_lock_param(slot);
    if (selected == 0 || selected == PARAM_OFF) {
      return false;
    }
    param_id = selected - 1;
    return true;
  }

  uint8_t selected_lock_menu_value(uint8_t slot) const {
    uint8_t param_id = 0;
    return selected_lock_param_id(slot, param_id) ? param_id : PARAM_OFF;
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

  void set_selected_lock_menu_value(uint8_t slot, uint8_t menu_value) {
    if (menu_value == PARAM_OFF) {
      set_selected_lock_param(slot, 0);
    } else {
      set_selected_lock_param(slot, menu_value + 1);
    }
  }

  bool selected_lock_param_info(uint8_t slot,
                                SeqExtStepLockParamInfo &info) const {
    info = SeqExtStepLockParamInfo();
#ifdef PLATFORM_TBD
    if (midi_track_) {
      if (slot >= MIDI_SEQ_NUM_LOCKS) return false;
      const auto &lock = midi_track_->seq_data.locks[slot];
      if (!lock.is_active()) return false;
      info.active = true;
      info.param_id = lock.parameter;
      info.default_value = lock.default_value;
      info.current_value = lock.default_value;
      info.resolution = (lock.flags & MIDI_SEQ_LOCK_FLAG_14BIT) ? 0x4000 : 128;

      if (lock.flags & MIDI_SEQ_LOCK_FLAG_P4_PARAM) {
        const TbdP4ParamDescriptor *desc =
            tbd_p4_sound_param_for_lock(midi_track_->p4_sound,
                                        (uint8_t)lock.parameter);
        if (!desc || !desc->is_visible()) return info.active;
        info.p4_param = true;
        info.sendable = desc->is_sendable();
        info.nrpn = desc->ctrl_type == TBD_P4_CTRLTYPE_NRPM;
        info.macro = desc->is_macro();
        info.type = desc->type;
        info.ctrl = desc->ctrl;
        info.ctrl_type = desc->ctrl_type;
        info.resolution = desc->resolution;
        info.min_value = desc->min_value;
        info.max_value = desc->max_value;
        info.default_value = desc->default_value;
        info.current_value = desc->value;
        return true;
      }

      info.sendable = true;
      info.nrpn = lock.type == MIDI_SEQ_LOCK_NRPN;
      info.ctrl_type = lock.type;
      return true;
    }
#endif
    uint8_t param_id = 0;
    if (!selected_lock_param_id(slot, param_id)) return false;
    info.active = true;
    info.param_id = param_id;
    info.sendable = param_id <= PARAM_CHP;
    info.ctrl = param_id;
    info.ctrl_type =
        param_id == PARAM_PB    ? SEQ_EXT_LOCK_CTRL_PITCH_BEND
        : param_id == PARAM_CHP ? SEQ_EXT_LOCK_CTRL_CHANNEL_PRESSURE
        : param_id == PARAM_PRG ? SEQ_EXT_LOCK_CTRL_PROGRAM_CHANGE
                                : SEQ_EXT_LOCK_CTRL_CC;
    return true;
  }

  bool copy_selected_lock_label(uint8_t slot, char *dst,
                                size_t dst_len) const {
    if (dst == nullptr || dst_len == 0) return false;
    dst[0] = '\0';
    SeqExtStepLockParamInfo info;
    if (!selected_lock_param_info(slot, info) || !info.active) return false;

#ifdef PLATFORM_TBD
    if (midi_track_ && info.p4_param) {
      const TbdP4ParamDescriptor *desc =
          tbd_p4_sound_param_for_lock(midi_track_->p4_sound,
                                      (uint8_t)info.param_id);
      if (desc && desc->shortname[0] != '\0') {
        copy_compact_label(desc->shortname, dst, dst_len);
        if (dst[0] != '\0') return true;
      }
      copy_param_number_label('P', (uint8_t)info.param_id, dst, dst_len);
      return true;
    }
#endif

    switch (info.ctrl_type) {
    case SEQ_EXT_LOCK_CTRL_PITCH_BEND:
      copy_literal("PB", dst, dst_len);
      break;
    case SEQ_EXT_LOCK_CTRL_CHANNEL_PRESSURE:
      copy_literal("CHP", dst, dst_len);
      break;
    case SEQ_EXT_LOCK_CTRL_PROGRAM_CHANGE:
      copy_literal("PRG", dst, dst_len);
      break;
    case SEQ_EXT_LOCK_CTRL_NRPN:
      copy_param_number_label('N', (uint8_t)info.param_id, dst, dst_len);
      break;
    case SEQ_EXT_LOCK_CTRL_RPN:
      copy_param_number_label('R', (uint8_t)info.param_id, dst, dst_len);
      break;
    default:
      copy_param_number_label('C', (uint8_t)info.param_id, dst, dst_len);
      break;
    }
    return true;
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
  static void copy_literal(const char *src, char *dst, size_t dst_len) {
    if (dst == nullptr || dst_len == 0) return;
    size_t out = 0;
    while (src && src[out] && out + 1 < dst_len) {
      dst[out] = src[out];
      out++;
    }
    dst[out] = '\0';
  }

  static void append_uint8(uint8_t value, char *dst, size_t dst_len,
                           size_t &out) {
    if (dst == nullptr || dst_len == 0) return;
    if (out + 1 >= dst_len) {
      dst[dst_len - 1] = '\0';
      return;
    }
    if (value >= 100 && out + 1 < dst_len) {
      dst[out++] = (char)('0' + value / 100);
      value %= 100;
    }
    if ((value >= 10 || out > 1) && out + 1 < dst_len) {
      dst[out++] = (char)('0' + value / 10);
      value %= 10;
    }
    if (out + 1 < dst_len) {
      dst[out++] = (char)('0' + value);
    }
    dst[out] = '\0';
  }

  static void copy_param_number_label(char prefix, uint8_t number, char *dst,
                                      size_t dst_len) {
    if (dst == nullptr || dst_len == 0) return;
    size_t out = 0;
    if (out + 1 >= dst_len) {
      dst[0] = '\0';
      return;
    }
    dst[out++] = prefix;
    append_uint8(number, dst, dst_len, out);
  }

#ifdef PLATFORM_TBD
  static void copy_compact_label(const char *src, char *dst, size_t dst_len) {
    if (dst == nullptr || dst_len == 0) return;
    dst[0] = '\0';
    if (src == nullptr || src[0] == '\0') return;

    size_t out = 0;
    while (*src && out + 1 < dst_len) {
      char c = *src++;
      if (c == '-' || c == '_' || c == ' ') {
        break;
      }
      if (c >= 'a' && c <= 'z') {
        c = (char)(c - ('a' - 'A'));
      }
      dst[out++] = c;
    }
    dst[out] = '\0';
  }
#endif

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
