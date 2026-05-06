/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQSTEPTRACKAPI_H__
#define SEQSTEPTRACKAPI_H__

#include "MCLSeq.h"
#include "SeqDefines.h"
#include <stddef.h>
#include <stdint.h>

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
      : backend_(LegacyMD), md_(&track)
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

  bool configure_driver_panel(char *info, size_t info_len,
                              uint8_t &pitch_encoder_max) const {
#if !defined(__AVR__) && defined(PLATFORM_TBD)
    if (backend_ == StepSeqTbd) {
      pitch_encoder_max = 1;
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

  uint8_t condition_count() const {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return STEPSEQ_NUM_TRIG_CONDITIONS;
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

  uint8_t get_track_lock_implicit(uint8_t step, uint8_t param_id) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return step_->get_track_lock_implicit(step, param_id);
    }
#endif
    return md_->get_track_lock_implicit(step, param_id);
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
#endif
    return false;
  }

  bool send_md_parameter_locks(uint8_t step) {
    if (backend_ != LegacyMD) {
      return false;
    }
    md_->send_parameter_locks(step, true);
    return true;
  }

private:
#if !defined(__AVR__) && defined(PLATFORM_TBD)
  static void copy_compact_label(const char *src, char *dst, size_t dst_len) {
    if (dst == nullptr || dst_len == 0) {
      return;
    }
    dst[0] = '\0';
    if (src == nullptr || src[0] == '\0') {
      return;
    }

    size_t out = 0;
    while (*src && out + 1 < dst_len && out < 5) {
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

  static void copy_tbd_label(const TbdP4SoundData &sound, char *dst,
                             size_t dst_len) {
    copy_compact_label(sound.machine_id, dst, dst_len);
    if (dst_len == 0 || dst[0] != '\0') {
      return;
    }
    copy_compact_label(sound.preset_name, dst, dst_len);
    if (dst[0] != '\0') {
      return;
    }
    copy_compact_label(sound.preset_id, dst, dst_len);
    if (dst[0] != '\0') {
      return;
    }
    if (dst_len > 2) {
      dst[0] = 'P';
      dst[1] = '4';
      dst[2] = '\0';
    }
  }
#endif

  Backend backend_;
  MDSeqTrack *md_;
#if !defined(__AVR__)
  StepSeqDataTrack *step_;
#endif
};

#endif /* SEQSTEPTRACKAPI_H__ */
