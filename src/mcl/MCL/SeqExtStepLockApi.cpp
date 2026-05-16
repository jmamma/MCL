/* Justin Mammarella jmamma@gmail.com 2018 */

#include "SeqExtStepLockApi.h"
#include "ExtSeqTrack.h"
#ifdef PLATFORM_TBD
#include "../Drivers/Generic/Sequencer/MidiSeqTrack.h"
#endif

bool SeqExtStepLockApi::delete_lock(seq_extstep_tick_t tick, uint8_t lock_idx,
                                    uint8_t value) {
#ifdef PLATFORM_TBD
  if (midi_track_) return midi_track_->del_lock(tick, lock_idx, value);
#endif
  return ext_track_->del_track_locks((uint16_t)tick, lock_idx, value);
}

bool SeqExtStepLockApi::add_lock(uint8_t step, uint16_t timing, uint8_t param,
                                 uint8_t value, bool slide,
                                 uint8_t lock_idx) {
#ifdef PLATFORM_TBD
  if (midi_track_) {
    return midi_track_->add_lock(step, timing, param, value, slide, lock_idx);
  }
#endif
  return ext_track_->set_track_locks(step, timing, param, value, slide,
                                     lock_idx);
}

bool SeqExtStepLockApi::replace_param_lock(uint8_t step, uint16_t timing,
                                           uint8_t param, uint8_t value,
                                           bool slide) {
#ifdef PLATFORM_TBD
  if (midi_track_) {
    uint8_t lock_idx = selected_lock_slot_for_param(param);
    if (lock_idx == 255) {
      for (uint8_t i = 0; i < MIDI_SEQ_NUM_LOCKS; i++) {
        if (!midi_track_->seq_data.locks[i].is_active()) {
          lock_idx = i;
          break;
        }
      }
    }
    if (lock_idx == 255) {
      return false;
    }
    return add_lock(step, timing, param, value, slide, lock_idx);
  }
#endif
  ext_track_->clear_track_locks(step, param, 255);
  return ext_track_->set_track_locks(step, timing, param, value, slide);
}

bool SeqExtStepLockApi::set_p4_param_lock(uint8_t step, uint16_t timing,
                                          uint8_t param, uint8_t value,
                                          bool slide) {
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

bool SeqExtStepLockApi::record_p4_param_lock(uint8_t param, uint8_t value,
                                             bool slide) {
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

bool SeqExtStepLockApi::p4_param_lock_value(uint8_t step, uint8_t param,
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

uint8_t SeqExtStepLockApi::count_lock_event(uint8_t step, uint8_t lock_idx) {
#ifdef PLATFORM_TBD
  if (midi_track_) return midi_track_->count_lock_event(step, lock_idx);
#endif
  return ext_track_->count_lock_event(step, lock_idx);
}

uint8_t SeqExtStepLockApi::selected_lock_param(uint8_t slot) const {
#ifdef PLATFORM_TBD
  if (midi_track_) return midi_track_->selected_lock_param(slot);
#endif
  return ext_track_->locks_params[slot];
}

bool SeqExtStepLockApi::selected_lock_param_id(uint8_t slot,
                                               uint8_t &param_id) const {
  uint8_t selected = selected_lock_param(slot);
  if (selected == 0) {
    return false;
  }
  param_id = selected - 1;
  return true;
}

uint8_t SeqExtStepLockApi::selected_lock_menu_value(uint8_t slot) const {
  uint8_t param_id = 0;
  return selected_lock_param_id(slot, param_id) ? param_id : PARAM_OFF;
}

#if !defined(__AVR__)
bool SeqExtStepLockApi::selected_lock_menu_editable(uint8_t slot) const {
  SeqExtStepLockParamInfo info;
  if (!selected_lock_param_info(slot, info) || !info.active) return true;
  if (info.learn || info.p4_param) return true;
  return (info.ctrl_type == SEQ_EXT_LOCK_CTRL_CC ||
          info.ctrl_type == SEQ_EXT_LOCK_CTRL_PITCH_BEND ||
          info.ctrl_type == SEQ_EXT_LOCK_CTRL_CHANNEL_PRESSURE ||
          info.ctrl_type == SEQ_EXT_LOCK_CTRL_PROGRAM_CHANGE) &&
         info.param_id <= PARAM_LEARN;
}
#endif

uint8_t SeqExtStepLockApi::lock_param_menu_max() const {
#ifdef PLATFORM_TBD
  if (midi_track_ && midi_track_->p4_sound.has_sendable_params()) {
    return PARAM_OFF;
  }
#endif
  return PARAM_LEARN;
}

bool SeqExtStepLockApi::lock_menu_value_info(
    uint8_t menu_value, SeqExtStepLockParamInfo &info) const {
#if defined(__AVR__)
  info.active = false;
  info.learn = false;
  info.param_id = 0;
  info.ctrl = 0;
  info.ctrl_type = SEQ_EXT_LOCK_CTRL_OFF;
  if (menu_value == PARAM_OFF) return false;
  if (menu_value == PARAM_LEARN) {
    info.active = true;
    info.learn = true;
    return true;
  }

  info.active = true;
  info.param_id = menu_value;
  info.ctrl = menu_value;
  info.ctrl_type =
      menu_value == PARAM_PB    ? SEQ_EXT_LOCK_CTRL_PITCH_BEND
      : menu_value == PARAM_CHP ? SEQ_EXT_LOCK_CTRL_CHANNEL_PRESSURE
      : menu_value == PARAM_PRG ? SEQ_EXT_LOCK_CTRL_PROGRAM_CHANGE
                                : SEQ_EXT_LOCK_CTRL_CC;
  return true;
#else
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
#endif
}

uint8_t SeqExtStepLockApi::normalize_lock_menu_value(uint8_t menu_value,
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

void SeqExtStepLockApi::set_selected_lock_param(uint8_t slot, uint8_t param) {
#ifdef PLATFORM_TBD
  if (midi_track_) {
    midi_track_->set_selected_lock_param(slot, param);
    return;
  }
#endif
  ext_track_->locks_params[slot] = param;
}

void SeqExtStepLockApi::set_selected_lock_menu_value(uint8_t slot,
                                                     uint8_t menu_value) {
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

bool SeqExtStepLockApi::set_selected_lock_control(uint8_t slot,
                                                  uint8_t ctrl_type,
                                                  uint16_t ctrl,
                                                  uint16_t default_value) {
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

bool SeqExtStepLockApi::selected_lock_param_info(
    uint8_t slot, SeqExtStepLockParamInfo &info) const {
#if defined(__AVR__)
  info.active = false;
  info.learn = false;
#else
  info = SeqExtStepLockParamInfo();
#endif
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
#if !defined(__AVR__)
  info.sendable = param_id <= PARAM_CHP;
#endif
  info.ctrl = param_id;
  info.ctrl_type =
      param_id == PARAM_PB    ? SEQ_EXT_LOCK_CTRL_PITCH_BEND
      : param_id == PARAM_CHP ? SEQ_EXT_LOCK_CTRL_CHANNEL_PRESSURE
      : param_id == PARAM_PRG ? SEQ_EXT_LOCK_CTRL_PROGRAM_CHANGE
                              : SEQ_EXT_LOCK_CTRL_CC;
  return true;
}

bool SeqExtStepLockApi::copy_selected_lock_label(uint8_t slot, char *dst,
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
      tbd_p4_copy_param_label(*desc, dst, dst_len);
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
#if !defined(__AVR__)
  case SEQ_EXT_LOCK_CTRL_NRPN:
    copy_param_number_label('N', info.param_id, dst, dst_len);
    break;
  case SEQ_EXT_LOCK_CTRL_RPN:
    copy_param_number_label('R', info.param_id, dst, dst_len);
    break;
#endif
  default:
    copy_param_number_label('C', info.param_id, dst, dst_len);
    break;
  }
  return true;
}

bool SeqExtStepLockApi::copy_lock_menu_value_label(uint8_t menu_value,
                                                   char *dst,
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
      tbd_p4_copy_param_label(*desc, dst, dst_len);
      if (dst[0] != '\0') return true;
    }
    copy_param_number_label(info.nrpn ? 'N' : 'P', info.param_id, dst,
                            dst_len);
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

uint8_t SeqExtStepLockApi::selected_lock_current_ui_value(uint8_t slot) const {
  SeqExtStepLockParamInfo info;
  if (!selected_lock_param_info(slot, info) || !info.active) return 64;
#if defined(__AVR__)
  (void)info;
  return 0;
#else
  return value7_from_param_value(info, info.current_value);
#endif
}

uint8_t SeqExtStepLockApi::lock_ui_value_from_control(uint8_t slot,
                                                      uint8_t ctrl_type,
                                                      uint16_t ctrl,
                                                      uint16_t value) const {
  SeqExtStepLockParamInfo info;
  if (!selected_lock_param_info(slot, info) || !info.active) {
    return value7_from_14(value);
  }
#if !defined(__AVR__)
  if (info.p4_param && info.ctrl == ctrl && info.ctrl_type == ctrl_type) {
    return value7_from_param_value(info, value);
  }
#else
  (void)ctrl;
#endif
  if (ctrl_type == SEQ_EXT_LOCK_CTRL_CC ||
      ctrl_type == SEQ_EXT_LOCK_CTRL_CHANNEL_PRESSURE ||
      ctrl_type == SEQ_EXT_LOCK_CTRL_PROGRAM_CHANGE ||
      ctrl_type == SEQ_EXT_LOCK_CTRL_POLY_PRESSURE) {
    return value > 127 ? 127 : (uint8_t)value;
  }
  return value7_from_14(value);
}

bool SeqExtStepLockApi::selected_lock_matches_control(uint8_t slot,
                                                      uint8_t ctrl_type,
                                                      uint16_t ctrl) const {
  SeqExtStepLockParamInfo info;
  if (!selected_lock_param_info(slot, info) || !info.active) return false;
  if (info.learn) return true;
#if !defined(__AVR__)
  if (info.p4_param) {
    return info.ctrl == ctrl && info.ctrl_type == ctrl_type;
  }
#endif
  return info.ctrl_type == ctrl_type && info.ctrl == ctrl;
}

uint8_t SeqExtStepLockApi::selected_lock_slot_for_param(uint8_t param) const {
#ifdef PLATFORM_TBD
  if (midi_track_) {
    for (uint8_t i = 0; i < MIDI_SEQ_NUM_LOCKS; i++) {
      SeqExtStepLockParamInfo info;
      if (selected_lock_param_info(i, info) && info.active &&
          !info.learn && info.ctrl_type == SEQ_EXT_LOCK_CTRL_CC &&
          info.ctrl == param) {
        return i;
      }
    }
    return 255;
  }
#endif
  for (uint8_t i = 0; i < NUM_LOCKS; i++) {
    uint8_t param_id = 0;
    if (selected_lock_param_id(i, param_id) && param_id == param) {
      return i;
    }
  }
  return 255;
}

bool SeqExtStepLockApi::copy_lock_value_label(uint8_t slot, uint8_t value,
                                              char *dst,
                                              size_t dst_len) const {
  if (dst == nullptr || dst_len == 0) return false;
  SeqExtStepLockParamInfo info;
  if (!selected_lock_param_info(slot, info) || !info.active || info.learn) {
    put_int16(value, dst, dst_len);
    return true;
  }
#if !defined(__AVR__)
  int16_t display_value = info.p4_param ? param_value_from_value7(info, value)
                                        : (int16_t)value;
#else
  int16_t display_value = (int16_t)value;
#endif
  put_int16(display_value, dst, dst_len);
  return true;
}

bool SeqExtStepLockApi::record_control_lock(uint8_t ctrl_type, uint16_t ctrl,
                                            uint16_t value, bool slide) {
#ifdef PLATFORM_TBD
  if (midi_track_) {
    uint8_t lock_param = 255;
    uint16_t value14 = value;
    uint16_t default_value14 = 0;
    uint8_t lock_flags = 0;
    if (find_p4_control(ctrl_type, ctrl, value, lock_param, value14,
                        default_value14)) {
      lock_flags = MIDI_SEQ_LOCK_FLAG_P4_PARAM | MIDI_SEQ_LOCK_FLAG_14BIT;
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
    if (value > 0x3FFF) value = 0x3FFF;
    ext_track_->record_track_locks(PARAM_PB, (uint8_t)(value >> 7), slide);
    return true;
  }
  if (ctrl_type == SEQ_EXT_LOCK_CTRL_CHANNEL_PRESSURE) {
    ext_track_->record_track_locks(PARAM_CHP, (uint8_t)value, false);
    return true;
  }
  return false;
}

void SeqExtStepLockApi::copy_literal(const char *src, char *dst,
                                     size_t dst_len) {
  if (dst == nullptr || dst_len == 0) return;
  size_t out = 0;
  while (src && src[out] && out + 1 < dst_len) {
    dst[out] = src[out];
    out++;
  }
  dst[out] = '\0';
}

void SeqExtStepLockApi::append_uint16(uint16_t value, char *dst,
                                      size_t dst_len, size_t &out) {
  if (dst == nullptr || dst_len == 0) return;
  if (out + 1 >= dst_len) {
    dst[dst_len - 1] = '\0';
    return;
  }
#if defined(__AVR__)
  if (value > 255) value = 255;
  uint8_t v = (uint8_t)value;
  if (v >= 100 && out + 1 < dst_len) {
    dst[out++] = (char)('0' + v / 100);
    v %= 100;
  }
  if ((v >= 10 || out > 1) && out + 1 < dst_len) {
    dst[out++] = (char)('0' + v / 10);
    v %= 10;
  }
  if (out + 1 < dst_len) {
    dst[out++] = (char)('0' + v);
  }
  dst[out] = '\0';
#else
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
#endif
}

void SeqExtStepLockApi::copy_param_number_label(char prefix, uint16_t number,
                                                char *dst, size_t dst_len) {
  if (dst == nullptr || dst_len == 0) return;
  size_t out = 0;
  if (out + 1 >= dst_len) {
    dst[0] = '\0';
    return;
  }
  dst[out++] = prefix;
  append_uint16(number, dst, dst_len, out);
}

void SeqExtStepLockApi::put_int16(int16_t value, char *dst, size_t dst_len) {
  if (dst == nullptr || dst_len == 0) return;
#if defined(__AVR__)
  if (value < 0) value = 0;
  if (value > 255) value = 255;
  uint8_t v = (uint8_t)value;
  size_t out = 0;
  if (v >= 100 && out + 1 < dst_len) {
    dst[out++] = (char)('0' + v / 100);
    v %= 100;
  }
  if ((v >= 10 || out > 0) && out + 1 < dst_len) {
    dst[out++] = (char)('0' + v / 10);
    v %= 10;
  }
  if (out + 1 < dst_len) {
    dst[out++] = (char)('0' + v);
  }
  dst[out] = '\0';
#else
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
#endif
}

uint16_t SeqExtStepLockApi::value14_from_value7(uint8_t value7) {
#if defined(__AVR__)
  if (value7 > 127) value7 = 127;
  return (uint16_t)value7 << 7;
#else
  if (value7 > 127) value7 = 127;
  return (uint16_t)(((uint32_t)value7 * 0x3FFFu + 63u) / 127u);
#endif
}

#if !defined(__AVR__)
int16_t SeqExtStepLockApi::param_value_from_value7(
    const SeqExtStepLockParamInfo &info, uint8_t value7) {
  if (info.max_value <= info.min_value) return info.min_value;
  uint16_t value14 = value14_from_value7(value7);
  int32_t range = (int32_t)info.max_value - (int32_t)info.min_value;
  int32_t scaled = (int32_t)info.min_value +
                   ((range * (int32_t)value14 + 0x1FFF) / 0x3FFF);
  if (scaled < info.min_value) scaled = info.min_value;
  if (scaled > info.max_value) scaled = info.max_value;
  return (int16_t)scaled;
}

uint8_t SeqExtStepLockApi::value7_from_param_value(
    const SeqExtStepLockParamInfo &info, int16_t value) {
  if (info.max_value <= info.min_value) return 0;
  if (value < info.min_value) value = info.min_value;
  if (value > info.max_value) value = info.max_value;
  uint16_t range = (uint16_t)(info.max_value - info.min_value);
  uint16_t offset = (uint16_t)(value - info.min_value);
  return (uint8_t)(((uint32_t)offset * 127u + (range / 2u)) / range);
}
#endif

#ifdef PLATFORM_TBD
uint8_t SeqExtStepLockApi::ctrl_type_to_midi_lock_type(uint8_t ctrl_type) {
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

bool SeqExtStepLockApi::find_p4_control(uint8_t ctrl_type, uint16_t ctrl,
                                        uint16_t value, uint8_t &lock_param,
                                        uint16_t &value14,
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

uint16_t SeqExtStepLockApi::value14_from_param_actual(
    const SeqExtStepLockParamInfo &info, int16_t value) {
  if (info.max_value <= info.min_value) return 0;
  if (value < info.min_value) value = info.min_value;
  if (value > info.max_value) value = info.max_value;
  uint32_t range = (uint16_t)(info.max_value - info.min_value);
  uint32_t offset = (uint16_t)(value - info.min_value);
  return (uint16_t)((offset * 0x3FFFu + (range / 2u)) / range);
}
#endif

uint8_t SeqExtStepLockApi::value7_from_14(uint16_t value14) {
#if defined(__AVR__)
  if (value14 > 0x3FFF) value14 = 0x3FFF;
  value14 = (value14 + 64u) >> 7;
  return value14 > 127 ? 127 : (uint8_t)value14;
#else
  if (value14 > 0x3FFF) value14 = 0x3FFF;
  return (uint8_t)(((uint32_t)value14 * 127u + 0x1FFFu) / 0x3FFFu);
#endif
}
