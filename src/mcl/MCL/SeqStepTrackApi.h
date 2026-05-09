/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQSTEPTRACKAPI_H__
#define SEQSTEPTRACKAPI_H__

#include "MCLSeq.h"
#if defined(PLATFORM_TBD)
#include "MCLSysConfig.h"
#include "MidiSetup.h"
#endif
#include "SeqDefines.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

extern uint8_t last_md_track;

struct SeqStepLockParamInfo {
  bool active = false;
  bool p4_param = false;
  bool sendable = false;
  bool nrpn = false;
  bool macro = false;
  uint8_t param_id = 0;
  uint8_t ctrl = 0;
  uint8_t ctrl_type = 0;
  uint16_t resolution = 128;
  int16_t min_value = 0;
  int16_t max_value = 127;
  int16_t default_value = 0;
  int16_t current_value = 0;
};

class SeqStepTrackApi {
public:
  enum Backend : uint8_t {
    LegacyMD,
#if !defined(__AVR__)
    StepSeqSpsx,
#if defined(PLATFORM_TBD)
    StepSeqTbd,
#endif
#endif
  };

  explicit SeqStepTrackApi(MDSeqTrack &track)
      :
#if !defined(__AVR__)
        backend_(LegacyMD),
#endif
        md_(&track)
#if !defined(__AVR__)
        ,
        step_(nullptr)
#endif
  {}

#if !defined(__AVR__)
  explicit SeqStepTrackApi(SPSXSeqTrack &track)
      : backend_(StepSeqSpsx), md_(nullptr), step_(&track) {}

#if defined(PLATFORM_TBD)
  explicit SeqStepTrackApi(TBDSeqTrack &track)
      : backend_(StepSeqTbd), md_(nullptr), step_(&track) {}
#endif
#endif

  bool is_stepseq() const {
#if !defined(__AVR__)
    return backend_ != LegacyMD;
#else
    return false;
#endif
  }
  bool is_tbd() const {
#if !defined(__AVR__) && defined(PLATFORM_TBD)
    return backend_ == StepSeqTbd;
#else
    return false;
#endif
  }

  bool uses_md_sound() const {
#if !defined(__AVR__) && defined(PLATFORM_TBD)
    return backend_ != StepSeqTbd;
#else
    return true;
#endif
  }

  bool uses_tbd_step_notes() const {
#if !defined(__AVR__) && defined(PLATFORM_TBD)
    return backend_ == StepSeqTbd &&
           tbd_p4_sound_uses_step_note(
               static_cast<TBDSeqTrack *>(step_)->p4_sound);
#else
    return false;
#endif
  }

  bool uses_step_pitch() const {
    return uses_md_sound() || uses_tbd_step_notes();
  }

  bool configure_driver_panel(char *info, size_t info_len,
                              uint8_t &pitch_encoder_max) const {
#if !defined(__AVR__) && defined(PLATFORM_TBD)
    if (backend_ == StepSeqTbd) {
      pitch_encoder_max = uses_tbd_step_notes() ? 127 : 1;
      copy_tbd_label(static_cast<TBDSeqTrack *>(step_)->p4_sound, info,
                     info_len);
      return true;
    }
#endif
    (void)info;
    (void)info_len;
    (void)pitch_encoder_max;
    return false;
  }

  uint8_t lock_param_count() const {
#if !defined(__AVR__)
#if defined(PLATFORM_TBD)
    if (backend_ == StepSeqTbd) {
      return TBD_P4_LOCK_PARAM_COUNT;
    }
#endif
    if (is_stepseq()) {
      return STEPSEQ_NUM_LOCKS;
    }
#endif
    return MD_PARAMS_PER_TRACK;
  }

  uint8_t lock_slot_count() const {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return STEPSEQ_NUM_LOCKS;
    }
#endif
    return NUM_LOCKS;
  }

  bool lock_param_info(uint8_t param_id, SeqStepLockParamInfo &info) const {
    memset(&info, 0, sizeof(info));
    if (param_id >= lock_param_count()) {
      return false;
    }

    info.active = true;
    info.sendable = true;
    info.param_id = param_id;
    info.ctrl = param_id;
    info.resolution = 128;
    info.min_value = 0;
    info.max_value = 127;

#if !defined(__AVR__) && defined(PLATFORM_TBD)
    if (backend_ == StepSeqTbd) {
      const TbdP4SoundData &sound = static_cast<TBDSeqTrack *>(step_)->p4_sound;
      if (param_id == TBD_P4_LOCK_NOTE_PARAM) {
        if (!tbd_p4_sound_uses_step_note(sound)) {
          memset(&info, 0, sizeof(info));
          return false;
        }
        info.active = true;
        info.p4_param = false;
        info.sendable = true;
        info.param_id = param_id;
        info.ctrl = param_id;
        info.resolution = 128;
        info.min_value = 0;
        info.max_value = 127;
        info.default_value = TBD_P4_DEFAULT_STEP_NOTE;
        info.current_value = TBD_P4_DEFAULT_STEP_NOTE;
        return true;
      }
      const TbdP4ParamDescriptor *desc =
          tbd_p4_sound_param_for_lock(sound, param_id);
      if (desc == nullptr || !desc->is_visible()) {
        memset(&info, 0, sizeof(info));
        return false;
      }
      info.p4_param = true;
      info.sendable = desc->is_sendable();
      info.nrpn = desc->ctrl_type == TBD_P4_CTRLTYPE_NRPM;
      info.macro = desc->is_macro();
      info.ctrl = desc->ctrl;
      info.ctrl_type = desc->ctrl_type;
      info.resolution = desc->resolution;
      info.min_value = desc->min_value;
      info.max_value = desc->max_value;
      info.default_value = desc->default_value;
      info.current_value = desc->value;
      return true;
    }
#endif

    info.default_value = current_lock_value(param_id);
    info.current_value = info.default_value;
    return true;
  }

  uint8_t current_lock_value(uint8_t param_id) const {
#if !defined(__AVR__) && defined(PLATFORM_TBD)
    if (backend_ == StepSeqTbd) {
      if (param_id == TBD_P4_LOCK_NOTE_PARAM) {
        return uses_tbd_step_notes() ? TBD_P4_DEFAULT_STEP_NOTE : 0;
      }
      const TbdP4ParamDescriptor *desc =
          tbd_p4_sound_param_for_lock(static_cast<TBDSeqTrack *>(step_)->p4_sound,
                                      param_id);
      if (desc == nullptr || !desc->is_visible()) {
        return 64;
      }
      return value7_from_param_value(*desc, desc->value);
    }
#endif
    if (param_id >= lock_param_count()) {
      return 0;
    }
    uint8_t track = track_index();
    if (track >= NUM_MD_TRACKS) {
      return 0;
    }
    return MD.kit.params[track][param_id];
  }

  bool copy_lock_param_label(uint8_t param_id, char *dst,
                             size_t dst_len) const {
    if (dst == nullptr || dst_len == 0) {
      return false;
    }
    dst[0] = '\0';

#if defined(__AVR__)
    if (param_id >= lock_param_count()) {
      return false;
    }
#else
    SeqStepLockParamInfo info;
    if (!lock_param_info(param_id, info) || !info.active) {
      return false;
    }
#endif

#if !defined(__AVR__) && defined(PLATFORM_TBD)
    if (backend_ == StepSeqTbd) {
      if (param_id == TBD_P4_LOCK_NOTE_PARAM && uses_tbd_step_notes()) {
        copy_fixed_label("NOT", dst, dst_len, 3);
        return dst[0] != '\0';
      }
      const TbdP4ParamDescriptor *desc =
          tbd_p4_sound_param_for_lock(static_cast<TBDSeqTrack *>(step_)->p4_sound,
                                      param_id);
      if (desc != nullptr && desc->shortname[0] != '\0') {
        tbd_p4_copy_param_label(*desc, dst, dst_len);
        if (dst[0] != '\0') {
          return true;
        }
      }
      copy_param_number_label(info.nrpn ? 'N' : 'P', param_id, dst, dst_len);
      return true;
    }
#endif

    uint8_t track = track_index();
    if (track >= NUM_MD_TRACKS) {
      return false;
    }
    const char *modelname = model_param_name(MD.kit.get_model(track), param_id);
    if (modelname != nullptr) {
      copy_fixed_label(modelname, dst, dst_len, 3);
      return dst[0] != '\0';
    }
    copy_param_number_label('P', param_id, dst, dst_len);
    return true;
  }

  uint8_t length() const {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return step_->length;
    }
#endif
    return md_->length;
  }

  uint8_t speed() const {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return step_->speed;
    }
#endif
    return md_->speed;
  }

  uint8_t step_count() const {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return step_->step_count;
    }
#endif
    return md_->step_count;
  }

  uint8_t track_index() const {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return step_->track_number;
    }
#endif
    return md_->track_number;
  }

  uint8_t mute_state() const {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return step_->mute_state;
    }
#endif
    return md_->mute_state;
  }

  void set_mute_state(uint8_t state) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->mute_state = state;
      return;
    }
#endif
    md_->mute_state = state;
  }

  void set_length(uint8_t len, bool expand = false) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->set_length(len, expand);
      return;
    }
#endif
    md_->set_length(len, expand);
  }

  bool request_speed_change(uint8_t new_speed) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      if (step_->count_down) {
        return false;
      }
      if (step_->speed == new_speed) {
        return false;
      }
      step_->set_speed(new_speed, step_->speed, true);
      return true;
    }
#endif
    return md_->request_speed_change(new_speed);
  }

  uint8_t condition_count() const {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return STEPSEQ_NUM_TRIG_CONDITIONS - 1;
    }
#endif
    return NUM_TRIG_CONDITIONS;
  }

  uint8_t step_conditional_from_knob(uint8_t condition, bool *plock) const {
    uint8_t num_cond = condition_count();
    if (condition > num_cond) {
      *plock = true;
      return condition - num_cond;
    }
    *plock = false;
    return condition;
  }

  uint8_t knob_conditional_from_step(uint8_t condition, bool plock) const {
    return plock ? condition + condition_count() : condition;
  }

  uint8_t timing_encoder_min() const {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return 0;
    }
#endif
    return 1;
  }

  uint8_t timing_encoder_center() const {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return 127;
    }
#endif
    return md_->get_timing_mid();
  }

  uint8_t timing_encoder_max() const {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return 254;
    }
#endif
    return md_->get_timing_mid() * 2 - 1;
  }

  uint8_t timing_display_mid() const {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return 0;
    }
#endif
    return md_->get_timing_mid();
  }

  uint8_t timing_encoder_for_step(uint8_t step) const {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return (uint8_t)(microtiming_for_step(step) + 127);
    }
#endif
    uint8_t timing = md_->timing[step];
    return timing == 0 ? md_->get_timing_mid() : timing;
  }

  int8_t microtiming_from_encoder(uint8_t encoder_value) const {
    return (int8_t)((int16_t)encoder_value - 127);
  }

  int8_t microtiming_for_step(uint8_t step) const {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return step_->microtiming[step];
    }
#endif
    return 0;
  }

  void rotate_left() {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->rotate_left();
      return;
    }
#endif
    md_->rotate_left();
  }

  void rotate_right() {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->rotate_right();
      return;
    }
#endif
    md_->rotate_right();
  }

  void reverse() {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->reverse();
      return;
    }
#endif
    md_->reverse();
  }

  void transpose(int8_t offset) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->transpose(offset);
      return;
    }
#endif
    md_->transpose(offset);
  }

  uint8_t mask_for(uint8_t ui_mask) const {
#if !defined(__AVR__)
    if (!is_stepseq()) {
      return ui_mask;
    }
    switch (ui_mask) {
    case MASK_LOCK: return STEPSEQ_MASK_LOCK;
    case MASK_MUTE: return STEPSEQ_MASK_MUTE;
    case MASK_SLIDE: return STEPSEQ_MASK_SLIDE;
    case MASK_LOCKS_ON_STEP: return STEPSEQ_MASK_LOCKS_ON_STEP;
    case MASK_PATTERN:
    default: return STEPSEQ_MASK_PATTERN;
    }
#else
    return ui_mask;
#endif
  }

  void get_mask(uint64_t *mask, uint8_t ui_mask) const {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->get_mask(mask, mask_for(ui_mask));
    } else {
#endif
      md_->get_mask(mask, ui_mask);
#if !defined(__AVR__)
    }
#endif
  }

  bool get_step(uint8_t step, uint8_t ui_mask) const {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return step_->get_step(step, mask_for(ui_mask));
    }
#endif
    return md_->get_step(step, ui_mask);
  }

  void set_step(uint8_t step, uint8_t ui_mask, bool value) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->set_step(step, mask_for(ui_mask), value);
    } else {
#endif
      md_->set_step(step, ui_mask, value);
#if !defined(__AVR__)
    }
#endif
  }

  uint8_t conditional_id(uint8_t step) const {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return step_->steps[step].cond_id;
    }
#endif
    return md_->steps[step].cond_id;
  }

  bool conditional_plock(uint8_t step) const {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return step_->steps[step].cond_plock;
    }
#endif
    return md_->steps[step].cond_plock;
  }

  void set_conditional(uint8_t step, uint8_t condition, bool plock) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->steps[step].cond_id = condition;
      step_->steps[step].cond_plock = plock;
    } else {
#endif
      md_->steps[step].cond_id = condition;
      md_->steps[step].cond_plock = plock;
#if !defined(__AVR__)
    }
#endif
  }

  void clear_conditional(uint8_t step) { set_conditional(step, 0, false); }

  void set_timing_from_encoder(uint8_t step, uint8_t encoder_value) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->microtiming[step] = microtiming_from_encoder(encoder_value);
    } else {
#endif
      md_->timing[step] = encoder_value;
#if !defined(__AVR__)
    }
#endif
  }

  void reset_timing(uint8_t step) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->microtiming[step] = 0;
    } else {
#endif
      md_->timing[step] = md_->get_timing_mid();
#if !defined(__AVR__)
    }
#endif
  }

  void clear_mute(uint8_t step) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->mute_mask &= ~(1ULL << step);
    } else {
#endif
      md_->mute_mask &= ~(1ULL << step);
#if !defined(__AVR__)
    }
#endif
  }

  void toggle_mute(uint8_t step) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->mute_mask ^= (1ULL << step);
    } else {
#endif
      md_->mute_mask ^= (1ULL << step);
#if !defined(__AVR__)
    }
#endif
  }

  uint64_t mute_mask() const {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return step_->mute_mask;
    }
#endif
    return md_->mute_mask;
  }

  void enable_step_locks(uint8_t step) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->enable_step_locks(step);
    } else {
#endif
      md_->enable_step_locks(step);
#if !defined(__AVR__)
    }
#endif
  }

  void clear_step_lock(uint8_t step, uint8_t param_id) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->clear_step_lock(step, param_id);
    } else {
#endif
      md_->clear_step_lock(step, param_id);
#if !defined(__AVR__)
    }
#endif
  }

  void clear_step_locks(uint8_t step) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->clear_step_locks(step);
      return;
    }
#endif
    md_->clear_step_locks(step);
  }

  void clear_param_locks(uint8_t param_id) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->clear_param_locks(param_id);
      return;
    }
#endif
    md_->clear_param_locks(param_id);
  }

  void clear_locks() {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->clear_locks();
      return;
    }
#endif
    md_->clear_locks();
  }

  bool set_track_locks(uint8_t step, uint8_t param_id, uint8_t value) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return step_->set_track_locks(step, param_id, value);
    }
#endif
    return md_->set_track_locks(step, param_id, value);
  }

  bool step_has_lock(uint8_t step, uint8_t lock_idx) const {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return step_->steps[step].is_lock(lock_idx);
    }
#endif
    return md_->steps[step].is_lock(lock_idx);
  }

  int8_t find_param(uint8_t param_id) const {
#if !defined(__AVR__)
    uint8_t lock_idx = is_stepseq() ? step_->find_param(param_id)
                                    : md_->find_param(param_id);
#else
    uint8_t lock_idx = md_->find_param(param_id);
#endif
    return lock_idx == 255 ? -1 : (int8_t)lock_idx;
  }

  uint8_t pitch_lock_param_id() const {
#if !defined(__AVR__) && defined(PLATFORM_TBD)
    if (backend_ == StepSeqTbd) {
      return TBD_P4_LOCK_NOTE_PARAM;
    }
#endif
    return 0;
  }

  uint8_t get_track_lock_implicit(uint8_t step, uint8_t param_id) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return step_->get_track_lock_implicit(step, param_id);
    }
#endif
    return md_->get_track_lock_implicit(step, param_id);
  }

  void clear_track(bool locks = true) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->clear_track(locks);
      return;
    }
#endif
    md_->clear_track(locks);
  }

  void clear_step(uint8_t step) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->clear_step(step);
      return;
    }
#endif
    MDSeqStep empty_step;
    memset(&empty_step, 0, sizeof(empty_step));
    md_->paste_step(step, &empty_step);
  }

  void clean_params() {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->clean_params();
      return;
    }
#endif
    md_->clean_params();
  }

  void copy_step(uint8_t step, MDSeqStep *md_step
#if !defined(__AVR__)
                 ,
                 StepSeqStep *stepseq_step
#endif
  ) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->copy_step(step, stepseq_step);
      return;
    }
#endif
    md_->copy_step(step, md_step);
  }

  void paste_step(uint8_t step, MDSeqStep *md_step
#if !defined(__AVR__)
                  ,
                  StepSeqStep *stepseq_step
#endif
  ) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->paste_step(step, stepseq_step);
      return;
    }
#endif
    md_->paste_step(step, md_step);
  }

  void set_track_pitch(uint8_t step, uint8_t pitch) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->set_track_pitch(step, pitch);
    } else {
#endif
      md_->set_track_pitch(step, pitch);
#if !defined(__AVR__)
    }
#endif
  }

  void get_step_locks(uint8_t step, uint8_t *params,
                      bool ignore_locks_disabled) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->get_step_locks(step, params, ignore_locks_disabled);
    } else {
#endif
      md_->get_step_locks(step, params, ignore_locks_disabled);
#if !defined(__AVR__)
    }
#endif
  }

  void record_track(uint8_t velocity) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->record_track(velocity);
    } else {
#endif
      md_->record_track(velocity);
#if !defined(__AVR__)
    }
#endif
  }

  void record_track_locks(uint8_t param_id, uint8_t value) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->record_track_locks(param_id, value);
    } else {
#endif
      md_->record_track_locks(param_id, value);
#if !defined(__AVR__)
    }
#endif
  }

  bool preview_step(uint8_t step) {
#if !defined(__AVR__)
    if (backend_ == StepSeqSpsx) {
      static_cast<SPSXSeqTrack *>(step_)->preview_step(step);
      return true;
    }
#if defined(PLATFORM_TBD)
    if (backend_ == StepSeqTbd) {
      return static_cast<TBDSeqTrack *>(step_)->preview_step(step);
    }
#endif
#endif
    return false;
  }

  bool send_md_parameter_locks(uint8_t step) {
#if !defined(__AVR__)
    if (backend_ != LegacyMD) {
      return false;
    }
#endif
    md_->send_parameter_locks(step, true);
    return true;
  }

private:
  static void append_uint8(uint8_t value, char *dst, size_t dst_len,
                           size_t &out) {
    if (dst == nullptr || dst_len == 0) {
      return;
    }
    if (value >= 100 && out + 1 < dst_len) {
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

  static void copy_param_number_label(char prefix, uint8_t number, char *dst,
                                      size_t dst_len) {
    if (dst == nullptr || dst_len == 0) {
      return;
    }
    if (dst_len < 2) {
      dst[0] = '\0';
      return;
    }
    size_t out = 0;
    dst[out++] = prefix;
    append_uint8(number, dst, dst_len, out);
  }

  static void copy_fixed_label(const char *src, char *dst, size_t dst_len,
                               uint8_t max_chars) {
    if (dst == nullptr || dst_len == 0) {
      return;
    }
    if (src == nullptr) {
      dst[0] = '\0';
      return;
    }
    size_t out = 0;
    while (*src && out + 1 < dst_len && out < max_chars) {
      dst[out++] = *src++;
    }
    dst[out] = '\0';
    if (out == 2 && out + 1 < dst_len) {
      dst[out++] = ' ';
      dst[out] = '\0';
    }
  }

#if !defined(__AVR__) && defined(PLATFORM_TBD)
  static uint8_t value7_from_param_value(const TbdP4ParamDescriptor &desc,
                                         int16_t value) {
    if (desc.max_value <= desc.min_value) {
      return 0;
    }
    if (value < desc.min_value) value = desc.min_value;
    if (value > desc.max_value) value = desc.max_value;
    uint16_t range = (uint16_t)(desc.max_value - desc.min_value);
    uint16_t offset = (uint16_t)(value - desc.min_value);
    return (uint8_t)(((uint32_t)offset * 127u + (range / 2u)) / range);
  }

  static void copy_tbd_label(const TbdP4SoundData &sound, char *dst,
                             size_t dst_len) {
    tbd_p4_copy_sound_notice(sound, dst, dst_len);
  }
#endif

#if !defined(__AVR__)
  Backend backend_;
#endif
  MDSeqTrack *md_;
#if !defined(__AVR__)
  StepSeqDataTrack *step_;
#endif
};

inline bool seq_step_api_uses_tbd_tracks() {
#if defined(PLATFORM_TBD)
  return mcl_cfg.grid_x_device == GRID_X_DEVICE_TBD;
#else
  return false;
#endif
}

inline SeqStepTrackApi seq_step_api_track_for(uint8_t track,
                                              bool use_tbd_tracks) {
#if defined(PLATFORM_TBD)
  if (use_tbd_tracks) {
    return SeqStepTrackApi(mcl_seq.tbd_tracks[track]);
  }
#else
  (void)use_tbd_tracks;
#endif
#if !defined(__AVR__)
  if (mcl_seq.using_spsx_tracks) {
    return SeqStepTrackApi(mcl_seq.spsx_tracks[track]);
  }
#endif
  return SeqStepTrackApi(mcl_seq.md_tracks[track]);
}

inline SeqStepTrackApi seq_step_api_active_track(bool use_tbd_tracks) {
  return seq_step_api_track_for(last_md_track, use_tbd_tracks);
}

inline uint8_t seq_step_api_track_count(bool use_tbd_tracks) {
#if defined(PLATFORM_TBD)
  if (use_tbd_tracks) {
    return mcl_seq.num_tbd_tracks;
  }
#else
  (void)use_tbd_tracks;
#endif
  return mcl_seq.num_md_tracks;
}

#endif /* SEQSTEPTRACKAPI_H__ */
