/* Justin Mammarella jmamma@gmail.com 2018 */

#include "Sequencer/SeqExtStepLockApi.h"
#include "ExtSeqTrack.h"
#include "Sequencer/PtcVoiceRouter.h"
#include "../Drivers/MD/MD.h"
#if !defined(__AVR__)
#include "../Drivers/Generic/Sequencer/MidiSeqTrack.h"
#endif
#ifdef PLATFORM_TBD
#include "../Drivers/TBD/TBDTrack.h"
#endif

#ifdef PLATFORM_TBD
namespace {

TbdP4SoundData *p4_sound_for_midi_track(MidiSeqTrack *track) {
  return track == nullptr ? nullptr : tbd_midi_runtime_sound(track->track_number);
}

const TbdP4SoundData *p4_sound_for_midi_track(const MidiSeqTrack *track) {
  return track == nullptr ? nullptr
                          : tbd_midi_runtime_sound_const(track->track_number);
}

uint8_t seq_ctrl_type_from_p4(uint8_t p4_ctrl_type) {
  return p4_ctrl_type == TBD_P4_CTRLTYPE_NRPM
             ? SEQ_EXT_LOCK_CTRL_NRPN
             : p4_ctrl_type == TBD_P4_CTRLTYPE_CC ? SEQ_EXT_LOCK_CTRL_CC
                                                  : SEQ_EXT_LOCK_CTRL_OFF;
}

uint8_t midi_lock_type_from_p4(uint8_t p4_ctrl_type) {
  return p4_ctrl_type == TBD_P4_CTRLTYPE_NRPM
             ? MIDI_SEQ_LOCK_NRPN
             : p4_ctrl_type == TBD_P4_CTRLTYPE_CC ? MIDI_SEQ_LOCK_CC
                                                  : MIDI_SEQ_LOCK_OFF;
}

uint16_t value14_from_u7(uint8_t value) {
  if (value > 127) value = 127;
  return (uint16_t)(((uint32_t)value * 0x3FFFu + 63u) / 127u);
}

uint8_t u7_from_value14(uint16_t value) {
  if (value > 0x3FFF) value = 0x3FFF;
  return (uint8_t)(((uint32_t)value * 127u + 0x1FFFu) / 0x3FFFu);
}

int16_t p4_actual_from_ui_value(const SeqExtStepLockParamInfo &info,
                                uint8_t value) {
  if (info.max_value <= info.min_value) return info.min_value;
  uint16_t value14 = value14_from_u7(value);
  int32_t range = (int32_t)info.max_value - (int32_t)info.min_value;
  int32_t scaled = (int32_t)info.min_value +
                   ((range * (int32_t)value14 + 0x1FFF) / 0x3FFF);
  if (scaled < info.min_value) scaled = info.min_value;
  if (scaled > info.max_value) scaled = info.max_value;
  return (int16_t)scaled;
}

uint8_t p4_ui_value_from_actual(const SeqExtStepLockParamInfo &info,
                                int16_t value) {
  if (info.max_value <= info.min_value) return 0;
  if (value < info.min_value) value = info.min_value;
  if (value > info.max_value) value = info.max_value;
  uint16_t range = (uint16_t)(info.max_value - info.min_value);
  uint16_t offset = (uint16_t)(value - info.min_value);
  return (uint8_t)(((uint32_t)offset * 127u + (range / 2u)) / range);
}

uint16_t p4_midi_value14_from_actual(const TbdP4ParamDescriptor &desc,
                                     int16_t value) {
  if (desc.ctrl_type == TBD_P4_CTRLTYPE_CC) {
    if (value < 0) value = 0;
    if (value > 127) value = 127;
    return value14_from_u7((uint8_t)value);
  }
  if (value < 0) value = 0;
  if (value > 0x3FFF) value = 0x3FFF;
  return (uint16_t)value;
}

uint8_t p4_ui_value_from_midi_value14(const SeqExtStepLockParamInfo &info,
                                      const TbdP4ParamDescriptor &desc,
                                      uint16_t value14) {
  int16_t actual = desc.ctrl_type == TBD_P4_CTRLTYPE_CC
                       ? (int16_t)u7_from_value14(value14)
                       : (int16_t)(value14 > 0x3FFF ? 0x3FFF : value14);
  return p4_ui_value_from_actual(info, actual);
}

bool lock_matches_p4_desc(const MidiSeqLockDefinition &lock,
                          const TbdP4ParamDescriptor &desc) {
  return lock.is_active() && lock.type == midi_lock_type_from_p4(desc.ctrl_type) &&
         lock.parameter == desc.ctrl;
}

const TbdP4ParamDescriptor *p4_param_for_lock(const TbdP4SoundData *sound,
                                              const MidiSeqLockDefinition &lock,
                                              uint8_t *param_idx = nullptr) {
  if (sound == nullptr) {
    return nullptr;
  }
  for (uint8_t i = 0; i < TBD_P4_LOCK_PARAM_COUNT; i++) {
    const TbdP4ParamDescriptor *desc = tbd_p4_sound_param_for_lock(*sound, i);
    if (desc != nullptr && desc->is_visible() && lock_matches_p4_desc(lock, *desc)) {
      if (param_idx != nullptr) {
        *param_idx = i;
      }
      return desc;
    }
  }
  return nullptr;
}

} // namespace
#endif

namespace {

constexpr uint8_t kRouteMdFirstCc = PTC_EXT_ROUTE_CHANNEL_BASE;
constexpr uint8_t kRouteMdParamCount = MD_PARAMS_PER_TRACK;
constexpr uint8_t kRouteMdMenuOff = kRouteMdParamCount;

#if defined(__AVR__)
static inline bool avr_route_md_param_mode(uint8_t channel) {
  return channel >= PTC_EXT_ROUTE_CHANNEL_BASE &&
         channel < PTC_EXT_ROUTE_CHANNEL_END;
}
#endif

bool route_md_param_from_cc(uint16_t cc, uint8_t &param) {
  if (cc < kRouteMdFirstCc ||
      cc >= (uint16_t)kRouteMdFirstCc + kRouteMdParamCount) {
    return false;
  }
  param = (uint8_t)(cc - kRouteMdFirstCc);
  return true;
}

bool ext_lock_control_from_param(uint8_t param_id, uint8_t &ctrl_type,
                                 uint16_t &ctrl) {
  if (param_id < 128) {
    ctrl_type = SEQ_EXT_LOCK_CTRL_CC;
    ctrl = param_id;
    return true;
  }
  ctrl = 0;
  if (param_id == PARAM_PRG) {
    ctrl_type = SEQ_EXT_LOCK_CTRL_PROGRAM_CHANGE;
    return true;
  }
  if (param_id == PARAM_PB) {
    ctrl_type = SEQ_EXT_LOCK_CTRL_PITCH_BEND;
    return true;
  }
  if (param_id == PARAM_CHP) {
    ctrl_type = SEQ_EXT_LOCK_CTRL_CHANNEL_PRESSURE;
    return true;
  }
  return false;
}

bool ext_lock_param_from_control(uint8_t ctrl_type, uint16_t ctrl,
                                 uint8_t &param_id) {
  if (ctrl_type == SEQ_EXT_LOCK_CTRL_CC && ctrl <= 127) {
    param_id = (uint8_t)ctrl;
    return true;
  }
  if (ctrl != 0) return false;
  if (ctrl_type == SEQ_EXT_LOCK_CTRL_PROGRAM_CHANGE) {
    param_id = PARAM_PRG;
    return true;
  }
  if (ctrl_type == SEQ_EXT_LOCK_CTRL_PITCH_BEND) {
    param_id = PARAM_PB;
    return true;
  }
  if (ctrl_type == SEQ_EXT_LOCK_CTRL_CHANNEL_PRESSURE) {
    param_id = PARAM_CHP;
    return true;
  }
  return false;
}

} // namespace

uint8_t SeqExtStepLockApi::track_channel() const {
#if !defined(__AVR__)
  if (midi_track_) return midi_track_->channel();
#endif
  return ext_track_ == nullptr ? 0 : ext_track_->channel;
}

bool SeqExtStepLockApi::route_md_param_mode() const {
#if defined(__AVR__)
  return false;
#else
  uint8_t channel = track_channel();
  return channel >= PTC_EXT_ROUTE_CHANNEL_BASE &&
         channel < PTC_EXT_ROUTE_CHANNEL_END;
#endif
}

bool SeqExtStepLockApi::route_md_selected_param_id(uint8_t slot,
                                                   uint8_t &param_id) const {
#if !defined(__AVR__)
  if (midi_track_) {
    if (slot >= MIDI_SEQ_NUM_LOCKS) return false;
    const auto &lock = midi_track_->seq_data.locks[slot];
    uint8_t param = 0;
    if (!lock.is_active() || lock.type != MIDI_SEQ_LOCK_CC ||
        !route_md_param_from_cc(lock.parameter, param)) {
      return false;
    }
    param_id = (uint8_t)lock.parameter;
    return true;
  }
#endif
  if (slot >= NUM_LOCKS) return false;
  uint8_t selected = selected_lock_param(slot);
  if (selected == 0) return false;
  uint8_t param = 0;
  param_id = selected - 1;
  return route_md_param_from_cc(param_id, param);
}

bool SeqExtStepLockApi::copy_route_md_param_label(uint8_t param, char *dst,
                                                  size_t dst_len) const {
  if (param >= kRouteMdParamCount) return false;
#if defined(__AVR__)
  copy_param_number_label('P', param, dst, dst_len);
  return true;
#else
  uint8_t source_track = ptc_route_channel_track(track_channel());
  const char *name = model_param_name(MD.kit.get_model(source_track), param);
  if (name != nullptr && name[0] != '\0') {
    copy_literal(name, dst, dst_len);
    return true;
  }
  copy_param_number_label('P', param, dst, dst_len);
  return true;
#endif
}

bool SeqExtStepLockApi::delete_lock(seq_extstep_tick_t tick, uint8_t lock_idx,
                                    uint8_t value) {
#if !defined(__AVR__)
  if (midi_track_) return midi_track_->del_lock(tick, lock_idx, value);
#endif
  return ext_track_->del_track_locks((uint16_t)tick, lock_idx, value);
}

void SeqExtStepLockApi::clear_step_locks(uint8_t step, uint8_t lock_idx) {
#if !defined(__AVR__)
  if (midi_track_) {
    midi_track_->clear_lock_step(step, lock_idx);
    return;
  }
#endif
  ext_track_->clear_track_locks_idx(step, lock_idx);
}

bool SeqExtStepLockApi::add_lock(uint8_t step, uint16_t timing, uint8_t param,
                                 uint8_t value, bool slide,
                                 uint8_t lock_idx) {
#if !defined(__AVR__)
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
#if !defined(__AVR__)
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
    const TbdP4SoundData *sound = p4_sound_for_midi_track(midi_track_);
    const TbdP4ParamDescriptor *desc =
        sound == nullptr ? nullptr : tbd_p4_sound_param_for_lock(*sound, param);
    if (desc == nullptr || !desc->is_sendable()) {
      return false;
    }
    SeqExtStepLockParamInfo info;
    if (!lock_menu_value_info(param, info)) {
      return false;
    }
    uint8_t type = midi_lock_type_from_p4(desc->ctrl_type);
    if (type == MIDI_SEQ_LOCK_OFF) {
      return false;
    }
    int16_t actual = p4_actual_from_ui_value(info, value);
    return midi_track_->set_lock_event(
        step, timing, type, desc->ctrl,
        p4_midi_value14_from_actual(*desc, actual), slide,
        p4_midi_value14_from_actual(*desc, desc->value),
        MIDI_SEQ_LOCK_FLAG_14BIT);
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
    const TbdP4SoundData *sound = p4_sound_for_midi_track(midi_track_);
    const TbdP4ParamDescriptor *desc =
        sound == nullptr ? nullptr : tbd_p4_sound_param_for_lock(*sound, param);
    if (desc == nullptr || !desc->is_sendable()) {
      return false;
    }
    SeqExtStepLockParamInfo info;
    if (!lock_menu_value_info(param, info)) {
      return false;
    }
    uint8_t type = midi_lock_type_from_p4(desc->ctrl_type);
    if (type == MIDI_SEQ_LOCK_OFF) {
      return false;
    }
    int16_t actual = p4_actual_from_ui_value(info, value);
    return midi_track_->record_lock(
        type, desc->ctrl, p4_midi_value14_from_actual(*desc, actual), slide,
        MIDI_SEQ_LOCK_FLAG_14BIT,
        p4_midi_value14_from_actual(*desc, desc->value));
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
    const TbdP4SoundData *sound = p4_sound_for_midi_track(midi_track_);
    const TbdP4ParamDescriptor *desc =
        sound == nullptr ? nullptr : tbd_p4_sound_param_for_lock(*sound, param);
    if (desc == nullptr || !desc->is_sendable()) {
      return false;
    }
    uint8_t lock_idx = 255;
    for (uint8_t i = 0; i < MIDI_SEQ_NUM_LOCKS; i++) {
      const auto &lock = midi_track_->seq_data.locks[i];
      if (lock_matches_p4_desc(lock, *desc)) {
        lock_idx = i;
        break;
      }
    }
    if (lock_idx == 255) return false;

    uint16_t start = 0;
    uint16_t end = 0;
    midi_track_->seq_data.locate(step, start, end);
    SeqExtStepLockParamInfo info;
    if (!lock_menu_value_info(param, info)) {
      return false;
    }
    for (uint16_t i = start; i < end; i++) {
      const auto &event = midi_track_->seq_data.events[i];
      if (event.type == MIDI_SEQ_EVENT_LOCK && event.target == lock_idx) {
        value = p4_ui_value_from_midi_value14(info, *desc, event.value);
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
#if !defined(__AVR__)
  if (midi_track_) return midi_track_->count_lock_event(step, lock_idx);
#endif
  return ext_track_->count_lock_event(step, lock_idx);
}

uint8_t SeqExtStepLockApi::selected_lock_param(uint8_t slot) const {
#if !defined(__AVR__)
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
#if defined(__AVR__)
  if (avr_route_md_param_mode(track_channel())) {
    uint8_t param_id = 0;
    if (!selected_lock_param_id(slot, param_id)) return kRouteMdMenuOff;
    return param_id < kRouteMdParamCount ? param_id : kRouteMdMenuOff;
  }
#endif
  if (route_md_param_mode()) {
    uint8_t param_id = 0;
    uint8_t route_param = 0;
    return route_md_selected_param_id(slot, param_id) &&
                   route_md_param_from_cc(param_id, route_param)
               ? route_param
               : kRouteMdMenuOff;
  }
#if !defined(__AVR__)
  if (midi_track_) {
    SeqExtStepLockParamInfo info;
    return selected_lock_param_info(slot, info) && info.active
               ? (uint8_t)info.param_id
               : PARAM_OFF;
  }
#endif
  uint8_t param_id = 0;
  return selected_lock_param_id(slot, param_id) ? param_id : PARAM_OFF;
}

#if !defined(__AVR__)
bool SeqExtStepLockApi::selected_lock_menu_editable(uint8_t slot) const {
  if (route_md_param_mode()) return true;
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
#if defined(__AVR__)
  if (avr_route_md_param_mode(track_channel())) {
    return kRouteMdMenuOff;
  }
#endif
  if (route_md_param_mode()) {
    return kRouteMdMenuOff;
  }
#ifdef PLATFORM_TBD
  const TbdP4SoundData *sound = p4_sound_for_midi_track(midi_track_);
  if (sound != nullptr && sound->has_sendable_params()) {
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
  if (route_md_param_mode()) {
    if (menu_value >= kRouteMdParamCount) return false;
    info.active = true;
    info.param_id = kRouteMdFirstCc + menu_value;
    info.ctrl = info.param_id;
    info.ctrl_type = SEQ_EXT_LOCK_CTRL_CC;
    return true;
  }
  if (menu_value == PARAM_OFF) return false;
  if (menu_value == PARAM_LEARN) {
    info.active = true;
    info.learn = true;
    return true;
  }

  info.active = true;
  info.param_id = menu_value;
  return ext_lock_control_from_param(menu_value, info.ctrl_type, info.ctrl);
#else
  info = SeqExtStepLockParamInfo();
  if (route_md_param_mode()) {
    if (menu_value >= kRouteMdParamCount) return false;
    info.active = true;
    info.param_id = kRouteMdFirstCc + menu_value;
    info.ctrl = info.param_id;
    info.ctrl_type = SEQ_EXT_LOCK_CTRL_CC;
    info.sendable = true;
    return true;
  }
  if (menu_value == PARAM_OFF) return false;
  if (menu_value == PARAM_LEARN) {
    info.active = true;
    info.learn = true;
    return true;
  }

#ifdef PLATFORM_TBD
  const TbdP4SoundData *sound = p4_sound_for_midi_track(midi_track_);
  if (sound != nullptr && sound->has_sendable_params()) {
    const TbdP4ParamDescriptor *desc =
        tbd_p4_sound_param_for_lock(*sound, menu_value);
    if (!desc || !desc->is_visible()) return false;
    info.active = true;
    info.p4_param = true;
    info.sendable = desc->is_sendable();
    info.nrpn = desc->ctrl_type == TBD_P4_CTRLTYPE_NRPM;
    info.macro = desc->is_macro();
    info.type = desc->type;
    info.ctrl = desc->ctrl;
    info.ctrl_type = seq_ctrl_type_from_p4(desc->ctrl_type);
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
  return ext_lock_control_from_param(menu_value, info.ctrl_type, info.ctrl);
#endif
}

uint8_t SeqExtStepLockApi::normalize_lock_menu_value(uint8_t menu_value,
                                                     uint8_t old_value) const {
#if defined(__AVR__)
  if (avr_route_md_param_mode(track_channel())) {
    return menu_value > kRouteMdMenuOff ? kRouteMdMenuOff : menu_value;
  }
#endif
  if (route_md_param_mode()) {
    return menu_value > kRouteMdMenuOff ? kRouteMdMenuOff : menu_value;
  }
#ifdef PLATFORM_TBD
  const TbdP4SoundData *sound = p4_sound_for_midi_track(midi_track_);
  if (sound != nullptr && sound->has_sendable_params()) {
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
#if !defined(__AVR__)
  if (midi_track_) {
    midi_track_->set_selected_lock_param(slot, param);
    return;
  }
#endif
  if (slot >= NUM_LOCKS) return;
  if (ext_track_->locks_params[slot] == param) return;
  ext_track_->locks_params[slot] = param;
#if MCL_FEATURE_HOST_EXTSTEP_SYNC
  ext_track_->locks_slide_data[slot].init();
  ext_track_->locks_slides_recalc = 255;
  ExtSeqTrack::epoch++;
#endif
}

void SeqExtStepLockApi::set_selected_lock_menu_value(uint8_t slot,
                                                     uint8_t menu_value) {
#if defined(__AVR__)
  if (avr_route_md_param_mode(track_channel())) {
    set_selected_lock_param(slot, menu_value >= kRouteMdParamCount
                                      ? 0
                                      : menu_value + 1);
    return;
  }
#endif
  if (route_md_param_mode()) {
    if (menu_value >= kRouteMdParamCount) {
      set_selected_lock_param(slot, 0);
      return;
    }
#if defined(__AVR__)
    set_selected_lock_param(slot, kRouteMdFirstCc + menu_value + 1);
#else
    set_selected_lock_control(slot, SEQ_EXT_LOCK_CTRL_CC,
                              kRouteMdFirstCc + menu_value);
#endif
    return;
  }
  if (menu_value == PARAM_OFF) {
    set_selected_lock_param(slot, 0);
    return;
  }
#ifdef PLATFORM_TBD
  if (midi_track_) {
    SeqExtStepLockParamInfo info;
    if (lock_menu_value_info(menu_value, info) && info.p4_param) {
      set_selected_lock_control(slot, info.ctrl_type, info.ctrl,
                                info.current_value);
      return;
    }
  }
#endif
  set_selected_lock_param(slot, menu_value + 1);
}

bool SeqExtStepLockApi::set_selected_lock_control(uint8_t slot,
                                                  uint8_t ctrl_type,
                                                  uint16_t ctrl,
                                                  uint16_t default_value) {
  if (route_md_param_mode()) {
    uint8_t route_param = 0;
    if (ctrl_type != SEQ_EXT_LOCK_CTRL_CC ||
        !route_md_param_from_cc(ctrl, route_param)) {
      return false;
    }
#if !defined(__AVR__)
    if (midi_track_) {
      return midi_track_->set_selected_lock_control(
          slot, MIDI_SEQ_LOCK_CC, ctrl,
          value14_from_value7((uint8_t)default_value));
    }
#endif
    set_selected_lock_param(slot, (uint8_t)ctrl + 1);
    return true;
  }
#if !defined(__AVR__)
  if (midi_track_) {
#ifdef PLATFORM_TBD
    uint8_t p4_lock_param = 255;
    uint16_t p4_value14 = 0;
    uint16_t p4_default_value14 = 0;
    if (find_p4_control(ctrl_type, ctrl, default_value, p4_lock_param,
                        p4_value14, p4_default_value14)) {
      (void)p4_lock_param;
      return midi_track_->set_selected_lock_control(
          slot, ctrl_type_to_midi_lock_type(ctrl_type), ctrl,
          p4_default_value14, MIDI_SEQ_LOCK_FLAG_14BIT);
    }
#endif

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
  uint8_t param_id = 0;
  if (!ext_lock_param_from_control(ctrl_type, ctrl, param_id)) return false;
  set_selected_lock_param(slot, param_id + 1);
  return true;
}

bool SeqExtStepLockApi::selected_lock_param_info(
    uint8_t slot, SeqExtStepLockParamInfo &info) const {
#if defined(__AVR__)
  info.active = false;
  info.learn = false;
#else
  info = SeqExtStepLockParamInfo();
#endif
  if (route_md_param_mode()) {
    uint8_t param_id = 0;
    if (!route_md_selected_param_id(slot, param_id)) return false;
    info.active = true;
    info.param_id = param_id;
    info.ctrl = param_id;
    info.ctrl_type = SEQ_EXT_LOCK_CTRL_CC;
#if !defined(__AVR__)
    info.sendable = true;
#endif
    return true;
  }
#if !defined(__AVR__)
  if (midi_track_) {
    if (slot >= MIDI_SEQ_NUM_LOCKS) return false;
    const auto &lock = midi_track_->seq_data.locks[slot];
    if (!lock.is_active()) return false;
    info.active = true;
    info.param_id = lock.parameter;
    info.default_value = lock.default_value;
    info.current_value = lock.default_value;
    info.resolution = (lock.flags & MIDI_SEQ_LOCK_FLAG_14BIT) ? 0x4000 : 128;

#ifdef PLATFORM_TBD
    uint8_t p4_param = 255;
    const TbdP4ParamDescriptor *desc =
        p4_param_for_lock(p4_sound_for_midi_track(midi_track_), lock,
                          &p4_param);
    if (desc != nullptr) {
      info.p4_param = true;
      info.sendable = desc->is_sendable();
      info.nrpn = desc->ctrl_type == TBD_P4_CTRLTYPE_NRPM;
      info.macro = desc->is_macro();
      info.type = desc->type;
      info.ctrl = desc->ctrl;
      info.ctrl_type = seq_ctrl_type_from_p4(desc->ctrl_type);
      info.param_id = p4_param;
      info.resolution = desc->resolution;
      info.min_value = desc->min_value;
      info.max_value = desc->max_value;
      info.default_value = desc->default_value;
      info.current_value = desc->value;
      return true;
    }
#endif

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
  return ext_lock_control_from_param(param_id, info.ctrl_type, info.ctrl);
}

bool SeqExtStepLockApi::copy_selected_lock_label(uint8_t slot, char *dst,
                                                 size_t dst_len) const {
  if (dst == nullptr || dst_len == 0) return false;
  dst[0] = '\0';
  if (route_md_param_mode()) {
#if defined(__AVR__)
    return false;
#else
    uint8_t param_id = 0;
    uint8_t route_param = 0;
    return route_md_selected_param_id(slot, param_id) &&
           route_md_param_from_cc(param_id, route_param) &&
           copy_route_md_param_label(route_param, dst, dst_len);
#endif
  }
  SeqExtStepLockParamInfo info;
  if (!selected_lock_param_info(slot, info) || !info.active) return false;
  if (info.learn) {
    copy_literal("LEARN", dst, dst_len);
    return true;
  }

#ifdef PLATFORM_TBD
  if (midi_track_ && info.p4_param) {
    const TbdP4SoundData *sound = p4_sound_for_midi_track(midi_track_);
    const TbdP4ParamDescriptor *desc =
        sound == nullptr
            ? nullptr
            : tbd_p4_sound_param_for_lock(*sound, (uint8_t)info.param_id);
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

bool SeqExtStepLockApi::copy_route_md_menu_value_label(uint8_t menu_value,
                                                       char *dst,
                                                       size_t dst_len) const {
  if (dst == nullptr || dst_len == 0 || !route_md_param_mode()) return false;
  dst[0] = '\0';
  if (menu_value >= kRouteMdParamCount) {
    copy_literal("OFF", dst, dst_len);
    return true;
  }
  return copy_route_md_param_label(menu_value, dst, dst_len);
}

bool SeqExtStepLockApi::copy_lock_menu_value_label(uint8_t menu_value,
                                                   char *dst,
                                                   size_t dst_len) const {
  if (dst == nullptr || dst_len == 0) return false;
  dst[0] = '\0';
  if (copy_route_md_menu_value_label(menu_value, dst, dst_len)) return true;
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
    const TbdP4SoundData *sound = p4_sound_for_midi_track(midi_track_);
    const TbdP4ParamDescriptor *desc =
        sound == nullptr
            ? nullptr
            : tbd_p4_sound_param_for_lock(*sound, (uint8_t)info.param_id);
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
#if defined(__AVR__)
  uint8_t param_id = 0;
  return selected_lock_param_id(slot, param_id) ? 0 : 64;
#else
  SeqExtStepLockParamInfo info;
  if (!selected_lock_param_info(slot, info) || !info.active) return 64;
  return value7_from_param_value(info, info.current_value);
#endif
}

uint8_t SeqExtStepLockApi::lock_ui_value_from_control(uint8_t slot,
                                                      uint8_t ctrl_type,
                                                      uint16_t ctrl,
                                                      uint16_t value) const {
#if defined(__AVR__)
  uint8_t param_id = 0;
  if (!selected_lock_param_id(slot, param_id)) {
    return value7_from_14(value);
  }
  (void)ctrl;
#else
  SeqExtStepLockParamInfo info;
  if (!selected_lock_param_info(slot, info) || !info.active) {
    return value7_from_14(value);
  }
  if (info.p4_param && info.ctrl == ctrl && info.ctrl_type == ctrl_type) {
    return value7_from_param_value(info, value);
  }
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
  if (route_md_param_mode()) {
    uint8_t param_id = 0;
    return ctrl_type == SEQ_EXT_LOCK_CTRL_CC &&
           route_md_selected_param_id(slot, param_id) && ctrl == param_id;
  }
#if defined(__AVR__)
  uint8_t param_id = 0;
  if (!selected_lock_param_id(slot, param_id)) return false;
  if (param_id == PARAM_LEARN) return true;
  uint8_t selected_ctrl_type = 0;
  uint16_t selected_ctrl = 0;
  if (!ext_lock_control_from_param(param_id, selected_ctrl_type,
                                   selected_ctrl)) {
    return false;
  }
  return selected_ctrl_type == ctrl_type && selected_ctrl == ctrl;
#else
  SeqExtStepLockParamInfo info;
  if (!selected_lock_param_info(slot, info) || !info.active) return false;
  if (info.learn) return true;
  if (info.p4_param) {
    return info.ctrl == ctrl && info.ctrl_type == ctrl_type;
  }
  return info.ctrl_type == ctrl_type && info.ctrl == ctrl;
#endif
}

uint8_t SeqExtStepLockApi::selected_lock_slot_for_param(uint8_t param) const {
#if !defined(__AVR__)
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
#if defined(__AVR__)
  (void)slot;
  put_int16(value, dst, dst_len);
  return true;
#else
  SeqExtStepLockParamInfo info;
  if (!selected_lock_param_info(slot, info) || !info.active || info.learn) {
    put_int16(value, dst, dst_len);
    return true;
  }
  int16_t display_value = info.p4_param ? param_value_from_value7(info, value)
                                        : (int16_t)value;
  put_int16(display_value, dst, dst_len);
  return true;
#endif
}

bool SeqExtStepLockApi::record_control_lock(uint8_t ctrl_type, uint16_t ctrl,
                                            uint16_t value, bool slide) {
  if (route_md_param_mode()) {
    uint8_t route_param = 0;
    if (ctrl_type != SEQ_EXT_LOCK_CTRL_CC ||
        !route_md_param_from_cc(ctrl, route_param)) {
      return false;
    }
  }
#if !defined(__AVR__)
  if (midi_track_) {
    uint8_t lock_param = 255;
    uint16_t value14 = value;
    uint16_t default_value14 = 0;
#ifdef PLATFORM_TBD
    if (find_p4_control(ctrl_type, ctrl, value, lock_param, value14,
                        default_value14)) {
      (void)lock_param;
      return midi_track_->record_lock(
          ctrl_type_to_midi_lock_type(ctrl_type), ctrl, value14, slide,
          MIDI_SEQ_LOCK_FLAG_14BIT, default_value14);
    }
#else
    (void)lock_param;
    (void)default_value14;
#endif
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

#if !defined(__AVR__)
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
#endif

#ifdef PLATFORM_TBD
bool SeqExtStepLockApi::find_p4_control(uint8_t ctrl_type, uint16_t ctrl,
                                        uint16_t value, uint8_t &lock_param,
                                        uint16_t &value14,
                                        uint16_t &default_value14) const {
  if (!midi_track_) return false;
  const TbdP4SoundData *sound = p4_sound_for_midi_track(midi_track_);
  if (sound == nullptr) return false;
  uint8_t p4_ctrl_type = ctrl_type == SEQ_EXT_LOCK_CTRL_NRPN
                             ? TBD_P4_CTRLTYPE_NRPM
                             : ctrl_type == SEQ_EXT_LOCK_CTRL_CC
                                   ? TBD_P4_CTRLTYPE_CC
                                   : TBD_P4_CTRLTYPE_UNKNOWN;
  if (p4_ctrl_type == TBD_P4_CTRLTYPE_UNKNOWN) return false;

  for (uint8_t i = 0; i < TBD_P4_LOCK_PARAM_COUNT; i++) {
    const TbdP4ParamDescriptor *desc =
        tbd_p4_sound_param_for_lock(*sound, i);
    if (!desc || !desc->is_sendable()) continue;
    if (desc->ctrl_type != p4_ctrl_type || desc->ctrl != ctrl) continue;

    SeqExtStepLockParamInfo info;
    lock_menu_value_info(i, info);
    lock_param = i;
    value14 = p4_midi_value14_from_actual(*desc, (int16_t)value);
    default_value14 = p4_midi_value14_from_actual(*desc, info.current_value);
    return true;
  }
  return false;
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
