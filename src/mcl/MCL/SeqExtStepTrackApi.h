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
  bool learn = false;
  uint8_t type = 0;
  uint16_t ctrl = 0;
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

  uint32_t step_tick(uint8_t step) const {
    return (uint32_t)step * ticks_per_step();
  }

  int32_t event_tick(uint8_t step, const SeqExtStepEvent &event) const {
    return (int32_t)step * ticks_per_step() + event.micro_timing -
           ticks_per_step();
  }

  uint8_t step_from_tick(uint32_t tick) const {
#ifdef PLATFORM_TBD
    if (midi_track_) return midi_track_->step_from_tick(tick);
#endif
    return tick / ticks_per_step();
  }

  uint16_t timing_from_tick(uint32_t tick) const {
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

  bool delete_note(uint32_t tick, uint32_t width, uint8_t note) {
#ifdef PLATFORM_TBD
    if (midi_track_) return midi_track_->del_note(tick, width, note);
#endif
    return ext_track_->del_note((uint16_t)tick, (uint16_t)width, note);
  }

  void add_note(uint32_t tick, uint32_t width, uint8_t note, uint8_t velocity,
                uint8_t condition) {
#ifdef PLATFORM_TBD
    if (midi_track_) {
      midi_track_->add_note(tick, width, note, velocity, condition);
      return;
    }
#endif
    ext_track_->add_note((uint16_t)tick, (uint16_t)width, note, velocity,
                         condition);
  }

  bool delete_lock(uint32_t tick, uint8_t lock_idx, uint8_t value) {
#ifdef PLATFORM_TBD
    if (midi_track_) return midi_track_->del_lock(tick, lock_idx, value);
#endif
    return ext_track_->del_track_locks((uint16_t)tick, lock_idx, value);
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

  bool set_p4_param_lock(uint8_t step, uint16_t timing, uint8_t param,
                         uint8_t value, bool slide) {
#ifdef PLATFORM_TBD
    if (midi_track_) {
      return midi_track_->set_p4_lock(step, timing, param, value, slide);
    }
#endif
    (void)step;
    (void)timing;
    (void)param;
    (void)value;
    (void)slide;
    return false;
  }

  bool record_p4_param_lock(uint8_t param, uint8_t value, bool slide) {
#ifdef PLATFORM_TBD
    if (midi_track_) {
      return midi_track_->record_p4_lock(param, value, slide);
    }
#endif
    (void)param;
    (void)value;
    (void)slide;
    return false;
  }

  bool p4_param_lock_value(uint8_t step, uint8_t param,
                           uint8_t &value) const {
#ifdef PLATFORM_TBD
    if (midi_track_) {
      uint8_t lock_idx = 255;
      for (uint8_t i = 0; i < MIDI_SEQ_NUM_LOCKS; i++) {
        const auto &lock = midi_track_->seq_data.locks[i];
        if (lock.is_active() && lock.type == MIDI_SEQ_LOCK_CC &&
            lock.parameter == param &&
            (lock.flags & MIDI_SEQ_LOCK_FLAG_P4_PARAM)) {
          lock_idx = i;
          break;
        }
      }
      if (lock_idx == 255) return false;

      uint16_t start = 0;
      uint16_t end = 0;
      midi_track_->seq_data.locate(step, start, end);
      for (uint16_t i = start; i < end; i++) {
        const auto &event = midi_track_->seq_data.events[i];
        if (event.type == MIDI_SEQ_EVENT_LOCK && event.target == lock_idx) {
          value = value7_from_14(event.value);
          return true;
        }
      }
      return false;
    }
#endif
    (void)step;
    (void)param;
    (void)value;
    return false;
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

  bool selected_lock_menu_editable(uint8_t slot) const {
    SeqExtStepLockParamInfo info;
    if (!selected_lock_param_info(slot, info) || !info.active) return true;
    if (info.learn || info.p4_param) return true;
    return (info.ctrl_type == SEQ_EXT_LOCK_CTRL_CC ||
            info.ctrl_type == SEQ_EXT_LOCK_CTRL_PITCH_BEND ||
            info.ctrl_type == SEQ_EXT_LOCK_CTRL_CHANNEL_PRESSURE ||
            info.ctrl_type == SEQ_EXT_LOCK_CTRL_PROGRAM_CHANGE) &&
           info.param_id <= PARAM_LEARN;
  }

  uint8_t lock_param_menu_max() const {
#ifdef PLATFORM_TBD
    if (midi_track_ && midi_track_->p4_sound.has_sendable_params()) {
      return PARAM_OFF;
    }
#endif
    return PARAM_LEARN;
  }

  bool lock_menu_value_info(uint8_t menu_value,
                            SeqExtStepLockParamInfo &info) const {
    info = SeqExtStepLockParamInfo();
    if (menu_value == PARAM_OFF) return false;
    if (menu_value == PARAM_LEARN) {
      info.active = true;
      info.learn = true;
      return true;
    }

#ifdef PLATFORM_TBD
    if (midi_track_ && midi_track_->p4_sound.has_sendable_params()) {
      const TbdP4ParamDescriptor *desc =
          tbd_p4_sound_param_for_lock(midi_track_->p4_sound, menu_value);
      if (!desc || !desc->is_visible()) return false;
      info.active = true;
      info.p4_param = true;
      info.sendable = desc->is_sendable();
      info.nrpn = desc->ctrl_type == TBD_P4_CTRLTYPE_NRPM;
      info.macro = desc->is_macro();
      info.type = desc->type;
      info.ctrl = desc->ctrl;
      info.ctrl_type = desc->ctrl_type;
      info.param_id = menu_value;
      info.resolution = desc->resolution;
      info.min_value = desc->min_value;
      info.max_value = desc->max_value;
      info.default_value = desc->default_value;
      info.current_value = desc->value;
      return true;
    }
#endif

    info.active = true;
    info.param_id = menu_value;
    info.sendable = menu_value <= PARAM_CHP;
    info.ctrl = menu_value;
    info.ctrl_type =
        menu_value == PARAM_PB    ? SEQ_EXT_LOCK_CTRL_PITCH_BEND
        : menu_value == PARAM_CHP ? SEQ_EXT_LOCK_CTRL_CHANNEL_PRESSURE
        : menu_value == PARAM_PRG ? SEQ_EXT_LOCK_CTRL_PROGRAM_CHANGE
                                  : SEQ_EXT_LOCK_CTRL_CC;
    return true;
  }

  uint8_t normalize_lock_menu_value(uint8_t menu_value,
                                    uint8_t old_value) const {
#ifdef PLATFORM_TBD
    if (midi_track_ && midi_track_->p4_sound.has_sendable_params()) {
      if (menu_value >= PARAM_OFF) return PARAM_OFF;

      SeqExtStepLockParamInfo info;
      if (lock_menu_value_info(menu_value, info)) return menu_value;

      if (menu_value >= old_value) {
        for (uint8_t v = menu_value; v < PARAM_OFF; v++) {
          if (lock_menu_value_info(v, info)) return v;
        }
        return PARAM_OFF;
      }

      for (int16_t v = menu_value; v >= 0; v--) {
        if (lock_menu_value_info((uint8_t)v, info)) return (uint8_t)v;
      }
      return PARAM_OFF;
    }
#endif
    if (menu_value > PARAM_LEARN) return PARAM_LEARN;
    return menu_value;
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
#ifdef PLATFORM_TBD
    } else if (midi_track_ && midi_track_->p4_sound.has_sendable_params()) {
      SeqExtStepLockParamInfo info;
      if (lock_menu_value_info(menu_value, info) && info.p4_param) {
        set_selected_lock_param(slot, menu_value + 1);
      }
#endif
    } else {
      set_selected_lock_param(slot, menu_value + 1);
    }
  }

  bool set_selected_lock_control(uint8_t slot, uint8_t ctrl_type,
                                 uint16_t ctrl, uint16_t default_value = 0) {
#ifdef PLATFORM_TBD
    if (midi_track_) {
      uint8_t p4_lock_param = 255;
      uint16_t p4_value14 = 0;
      uint16_t p4_default_value14 = 0;
      if (find_p4_control(ctrl_type, ctrl, default_value, p4_lock_param,
                          p4_value14, p4_default_value14)) {
        return midi_track_->set_selected_lock_control(
            slot, MIDI_SEQ_LOCK_CC, p4_lock_param, p4_default_value14,
            MIDI_SEQ_LOCK_FLAG_P4_PARAM | MIDI_SEQ_LOCK_FLAG_14BIT);
      }

      uint8_t type = ctrl_type_to_midi_lock_type(ctrl_type);
      if (type == MIDI_SEQ_LOCK_OFF) return false;
      uint16_t lock_default = default_value;
      if (type == MIDI_SEQ_LOCK_CC ||
          type == MIDI_SEQ_LOCK_CHANNEL_PRESSURE ||
          type == MIDI_SEQ_LOCK_PROGRAM_CHANGE ||
          type == MIDI_SEQ_LOCK_POLY_PRESSURE) {
        lock_default = value14_from_value7((uint8_t)default_value);
      }
      return midi_track_->set_selected_lock_control(slot, type, ctrl,
                                                    lock_default);
    }
#endif
    if (ctrl_type == SEQ_EXT_LOCK_CTRL_CC && ctrl <= 127) {
      set_selected_lock_param(slot, (uint8_t)ctrl + 1);
      return true;
    }
    return false;
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
      info.ctrl = lock.parameter;
      info.ctrl_type = lock.type;
      return true;
    }
#endif
    uint8_t param_id = 0;
    if (!selected_lock_param_id(slot, param_id)) return false;
    if (param_id == PARAM_LEARN) {
      info.active = true;
      info.learn = true;
      info.param_id = param_id;
      return true;
    }
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
    if (info.learn) {
      copy_literal("LEARN", dst, dst_len);
      return true;
    }

#ifdef PLATFORM_TBD
    if (midi_track_ && info.p4_param) {
      const TbdP4ParamDescriptor *desc =
          tbd_p4_sound_param_for_lock(midi_track_->p4_sound,
                                      (uint8_t)info.param_id);
      if (desc && desc->shortname[0] != '\0') {
        copy_compact_label(desc->shortname, dst, dst_len);
        if (dst[0] != '\0') return true;
      }
      copy_param_number_label('P', info.param_id, dst, dst_len);
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
      copy_param_number_label('N', info.param_id, dst, dst_len);
      break;
    case SEQ_EXT_LOCK_CTRL_RPN:
      copy_param_number_label('R', info.param_id, dst, dst_len);
      break;
    default:
      copy_param_number_label('C', info.param_id, dst, dst_len);
      break;
    }
    return true;
  }

  bool copy_lock_menu_value_label(uint8_t menu_value, char *dst,
                                  size_t dst_len) const {
    if (dst == nullptr || dst_len == 0) return false;
    dst[0] = '\0';
    if (menu_value == PARAM_OFF) {
      copy_literal("OFF", dst, dst_len);
      return true;
    }
    SeqExtStepLockParamInfo info;
    if (!lock_menu_value_info(menu_value, info) || !info.active) {
      return false;
    }
    if (info.learn) {
      copy_literal("LEARN", dst, dst_len);
      return true;
    }
#ifdef PLATFORM_TBD
    if (midi_track_ && info.p4_param) {
      const TbdP4ParamDescriptor *desc =
          tbd_p4_sound_param_for_lock(midi_track_->p4_sound,
                                      (uint8_t)info.param_id);
      if (desc && desc->shortname[0] != '\0') {
        copy_compact_label(desc->shortname, dst, dst_len);
        if (dst[0] != '\0') return true;
      }
      copy_param_number_label(info.nrpn ? 'N' : 'P', info.param_id,
                              dst, dst_len);
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
    default:
      copy_param_number_label('C', info.param_id, dst, dst_len);
      break;
    }
    return true;
  }

  uint8_t selected_lock_current_ui_value(uint8_t slot) const {
    SeqExtStepLockParamInfo info;
    if (!selected_lock_param_info(slot, info) || !info.active) return 64;
    return value7_from_param_value(info, info.current_value);
  }

  uint8_t lock_ui_value_from_control(uint8_t slot, uint8_t ctrl_type,
                                     uint16_t ctrl, uint16_t value) const {
    SeqExtStepLockParamInfo info;
    if (!selected_lock_param_info(slot, info) || !info.active) {
      return value7_from_14(value);
    }
    if (info.p4_param && info.ctrl == ctrl && info.ctrl_type == ctrl_type) {
      return value7_from_param_value(info, value);
    }
    if (ctrl_type == SEQ_EXT_LOCK_CTRL_CC ||
        ctrl_type == SEQ_EXT_LOCK_CTRL_CHANNEL_PRESSURE ||
        ctrl_type == SEQ_EXT_LOCK_CTRL_PROGRAM_CHANGE ||
        ctrl_type == SEQ_EXT_LOCK_CTRL_POLY_PRESSURE) {
      return value > 127 ? 127 : (uint8_t)value;
    }
    return value7_from_14(value);
  }

  bool selected_lock_matches_control(uint8_t slot, uint8_t ctrl_type,
                                     uint16_t ctrl) const {
    SeqExtStepLockParamInfo info;
    if (!selected_lock_param_info(slot, info) || !info.active) return false;
    if (info.learn) return true;
    if (info.p4_param) {
      return info.ctrl == ctrl && info.ctrl_type == ctrl_type;
    }
    return info.ctrl_type == ctrl_type && info.ctrl == ctrl;
  }

  bool copy_lock_value_label(uint8_t slot, uint8_t value, char *dst,
                             size_t dst_len) const {
    if (dst == nullptr || dst_len == 0) return false;
    SeqExtStepLockParamInfo info;
    if (!selected_lock_param_info(slot, info) || !info.active ||
        info.learn) {
      put_int16(value, dst, dst_len);
      return true;
    }
    int16_t display_value = info.p4_param
                                ? param_value_from_value7(info, value)
                                : (int16_t)value;
    put_int16(display_value, dst, dst_len);
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

  uint8_t channel() const {
#ifdef PLATFORM_TBD
    if (midi_track_) return midi_track_->channel();
#endif
    return ext_track_->channel;
  }

  void note_on(uint8_t note, uint8_t velocity,
               MidiUartClass *uart_ = nullptr) {
#ifdef PLATFORM_TBD
    if (midi_track_) {
      midi_track_->note_on(note, velocity, uart_);
      return;
    }
#endif
    ext_track_->note_on(note, velocity, uart_);
  }

  void note_off(uint8_t note, uint8_t velocity = 0,
                MidiUartClass *uart_ = nullptr) {
#ifdef PLATFORM_TBD
    if (midi_track_) {
      midi_track_->note_off(note, velocity, uart_);
      return;
    }
#endif
    ext_track_->note_off(note, velocity, uart_);
  }

  void send_cc(uint8_t cc, uint8_t value, MidiUartClass *uart_ = nullptr) {
#ifdef PLATFORM_TBD
    if (midi_track_) {
      midi_track_->send_cc(cc, value, uart_);
      return;
    }
#endif
    ext_track_->send_cc(cc, value, uart_);
  }

  void pitch_bend(uint16_t value, MidiUartClass *uart_ = nullptr) {
#ifdef PLATFORM_TBD
    if (midi_track_) {
      midi_track_->pitch_bend(value, uart_);
      return;
    }
#endif
    ext_track_->pitch_bend(value, uart_);
  }

  void channel_pressure(uint8_t pressure, MidiUartClass *uart_ = nullptr) {
#ifdef PLATFORM_TBD
    if (midi_track_) {
      midi_track_->channel_pressure(pressure, uart_);
      return;
    }
#endif
    ext_track_->channel_pressure(pressure, uart_);
  }

  void after_touch(uint8_t note, uint8_t pressure,
                   MidiUartClass *uart_ = nullptr) {
#ifdef PLATFORM_TBD
    if (midi_track_) {
      midi_track_->after_touch(note, pressure, uart_);
      return;
    }
#endif
    ext_track_->after_touch(note, pressure, uart_);
  }

  void buffer_notesoff() {
#ifdef PLATFORM_TBD
    if (midi_track_) {
      midi_track_->buffer_notesoff();
      return;
    }
#endif
    ext_track_->buffer_notesoff();
  }

  void init_notes_on() {
#ifdef PLATFORM_TBD
    if (midi_track_) {
      midi_track_->init_notes_on();
      return;
    }
#endif
    ext_track_->init_notes_on();
  }

  void update_param(uint8_t param_id, uint8_t value) {
#ifdef PLATFORM_TBD
    if (midi_track_) {
      (void)param_id;
      (void)value;
      return;
    }
#endif
    ext_track_->update_param(param_id, value);
  }

  void record_note_on(uint8_t note, uint8_t velocity) {
#ifdef PLATFORM_TBD
    if (midi_track_) {
      midi_track_->record_track_noteon(note, velocity);
      return;
    }
#endif
    ext_track_->record_track_noteon(note, velocity);
  }

  void record_note_off(uint8_t note) {
#ifdef PLATFORM_TBD
    if (midi_track_) {
      midi_track_->record_track_noteoff(note);
      return;
    }
#endif
    ext_track_->record_track_noteoff(note);
  }

  bool record_control_lock(uint8_t ctrl_type, uint16_t ctrl, uint16_t value,
                           bool slide) {
#ifdef PLATFORM_TBD
    if (midi_track_) {
      uint8_t lock_param = 255;
      uint16_t value14 = value;
      uint16_t default_value14 = 0;
      uint8_t lock_flags = 0;
      if (find_p4_control(ctrl_type, ctrl, value, lock_param, value14,
                          default_value14)) {
        lock_flags = MIDI_SEQ_LOCK_FLAG_P4_PARAM |
                     MIDI_SEQ_LOCK_FLAG_14BIT;
        return midi_track_->record_lock(MIDI_SEQ_LOCK_CC, lock_param, value14,
                                        slide, lock_flags, default_value14);
      }
      uint8_t midi_type = ctrl_type_to_midi_lock_type(ctrl_type);
      if (midi_type == MIDI_SEQ_LOCK_OFF) return false;
      if (midi_type == MIDI_SEQ_LOCK_CC ||
          midi_type == MIDI_SEQ_LOCK_CHANNEL_PRESSURE ||
          midi_type == MIDI_SEQ_LOCK_PROGRAM_CHANGE ||
          midi_type == MIDI_SEQ_LOCK_POLY_PRESSURE) {
        value14 = value14_from_value7((uint8_t)value);
      }
      return midi_track_->record_lock(midi_type, ctrl, value14, slide, 0);
    }
#endif
    if (ctrl_type == SEQ_EXT_LOCK_CTRL_CC && ctrl <= 127) {
      ext_track_->record_track_locks((uint8_t)ctrl, (uint8_t)value, slide);
      return true;
    }
    if (ctrl_type == SEQ_EXT_LOCK_CTRL_PITCH_BEND) {
      ext_track_->record_track_locks(PARAM_PB, value7_from_14(value), slide);
      return true;
    }
    if (ctrl_type == SEQ_EXT_LOCK_CTRL_CHANNEL_PRESSURE) {
      ext_track_->record_track_locks(PARAM_CHP, (uint8_t)value, false);
      return true;
    }
    return false;
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

  static void append_uint16(uint16_t value, char *dst, size_t dst_len,
                            size_t &out) {
    if (dst == nullptr || dst_len == 0) return;
    if (out + 1 >= dst_len) {
      dst[dst_len - 1] = '\0';
      return;
    }
    if (value > 9999) value = 9999;
    if (value >= 1000 && out + 1 < dst_len) {
      dst[out++] = (char)('0' + value / 1000);
      value %= 1000;
    }
    if ((value >= 100 || out > 1) && out + 1 < dst_len) {
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

  static void copy_param_number_label(char prefix, uint16_t number, char *dst,
                                      size_t dst_len) {
    if (dst == nullptr || dst_len == 0) return;
    size_t out = 0;
    if (out + 1 >= dst_len) {
      dst[0] = '\0';
      return;
    }
    dst[out++] = prefix;
    append_uint16(number, dst, dst_len, out);
  }

  static void put_int16(int16_t value, char *dst, size_t dst_len) {
    if (dst == nullptr || dst_len == 0) return;
    size_t out = 0;
    if (value < 0 && out + 1 < dst_len) {
      dst[out++] = '-';
      value = -value;
    }
    if (value > 9999) value = 9999;
    if (value >= 1000 && out + 1 < dst_len) {
      dst[out++] = (char)('0' + value / 1000);
      value %= 1000;
    }
    if ((value >= 100 || out > 0) && out + 1 < dst_len) {
      dst[out++] = (char)('0' + value / 100);
      value %= 100;
    }
    if ((value >= 10 || out > 0) && out + 1 < dst_len) {
      dst[out++] = (char)('0' + value / 10);
      value %= 10;
    }
    if (out + 1 < dst_len) {
      dst[out++] = (char)('0' + value);
    }
    dst[out] = '\0';
  }

  static uint16_t value14_from_value7(uint8_t value7) {
    if (value7 > 127) value7 = 127;
    return (uint16_t)(((uint32_t)value7 * 0x3FFFu + 63u) / 127u);
  }

  static int16_t param_value_from_value7(const SeqExtStepLockParamInfo &info,
                                         uint8_t value7) {
    if (info.max_value <= info.min_value) return info.min_value;
    uint16_t value14 = value14_from_value7(value7);
    int32_t range = (int32_t)info.max_value - (int32_t)info.min_value;
    int32_t scaled = (int32_t)info.min_value +
                     ((range * (int32_t)value14 + 0x1FFF) / 0x3FFF);
    if (scaled < info.min_value) scaled = info.min_value;
    if (scaled > info.max_value) scaled = info.max_value;
    return (int16_t)scaled;
  }

  static uint8_t value7_from_param_value(const SeqExtStepLockParamInfo &info,
                                         int16_t value) {
    if (info.max_value <= info.min_value) return 0;
    if (value < info.min_value) value = info.min_value;
    if (value > info.max_value) value = info.max_value;
    uint16_t range = (uint16_t)(info.max_value - info.min_value);
    uint16_t offset = (uint16_t)(value - info.min_value);
    return (uint8_t)(((uint32_t)offset * 127u + (range / 2u)) / range);
  }

#ifdef PLATFORM_TBD
  static uint8_t ctrl_type_to_midi_lock_type(uint8_t ctrl_type) {
    switch (ctrl_type) {
    case SEQ_EXT_LOCK_CTRL_CC:
      return MIDI_SEQ_LOCK_CC;
    case SEQ_EXT_LOCK_CTRL_NRPN:
      return MIDI_SEQ_LOCK_NRPN;
    case SEQ_EXT_LOCK_CTRL_RPN:
      return MIDI_SEQ_LOCK_RPN;
    case SEQ_EXT_LOCK_CTRL_PITCH_BEND:
      return MIDI_SEQ_LOCK_PITCH_BEND;
    case SEQ_EXT_LOCK_CTRL_CHANNEL_PRESSURE:
      return MIDI_SEQ_LOCK_CHANNEL_PRESSURE;
    case SEQ_EXT_LOCK_CTRL_PROGRAM_CHANGE:
      return MIDI_SEQ_LOCK_PROGRAM_CHANGE;
    case SEQ_EXT_LOCK_CTRL_POLY_PRESSURE:
      return MIDI_SEQ_LOCK_POLY_PRESSURE;
    default:
      return MIDI_SEQ_LOCK_OFF;
    }
  }

  bool find_p4_control(uint8_t ctrl_type, uint16_t ctrl, uint16_t value,
                       uint8_t &lock_param, uint16_t &value14,
                       uint16_t &default_value14) const {
    if (!midi_track_) return false;
    uint8_t p4_ctrl_type = ctrl_type == SEQ_EXT_LOCK_CTRL_NRPN
                               ? TBD_P4_CTRLTYPE_NRPM
                               : ctrl_type == SEQ_EXT_LOCK_CTRL_CC
                                     ? TBD_P4_CTRLTYPE_CC
                                     : TBD_P4_CTRLTYPE_UNKNOWN;
    if (p4_ctrl_type == TBD_P4_CTRLTYPE_UNKNOWN) return false;

    for (uint8_t i = 0; i < TBD_P4_LOCK_PARAM_COUNT; i++) {
      const TbdP4ParamDescriptor *desc =
          tbd_p4_sound_param_for_lock(midi_track_->p4_sound, i);
      if (!desc || !desc->is_sendable()) continue;
      if (desc->ctrl_type != p4_ctrl_type || desc->ctrl != ctrl) continue;

      SeqExtStepLockParamInfo info;
      lock_menu_value_info(i, info);
      lock_param = i;
      value14 = value14_from_param_actual(info, (int16_t)value);
      default_value14 = value14_from_param_actual(info, info.current_value);
      return true;
    }
    return false;
  }

  static uint16_t value14_from_param_actual(
      const SeqExtStepLockParamInfo &info, int16_t value) {
    if (info.max_value <= info.min_value) return 0;
    if (value < info.min_value) value = info.min_value;
    if (value > info.max_value) value = info.max_value;
    uint32_t range = (uint16_t)(info.max_value - info.min_value);
    uint32_t offset = (uint16_t)(value - info.min_value);
    return (uint16_t)((offset * 0x3FFFu + (range / 2u)) / range);
  }

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
