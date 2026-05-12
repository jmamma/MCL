/* Justin Mammarella jmamma@gmail.com 2018 */

#include "SeqStepTrackRef.h"

namespace SeqStepTrackBackend {

inline MDSeqTrack *md(void *track) { return static_cast<MDSeqTrack *>(track); }

inline const MDSeqTrack *md(const void *track) {
  return static_cast<const MDSeqTrack *>(track);
}

inline bool md_uses_kit_sound(const void *track) {
  (void)track;
  return true;
}

inline bool md_selects_track_locally(const void *track) {
  (void)track;
  return false;
}

inline uint8_t md_lock_slot_count(const void *track) {
  (void)track;
  return NUM_LOCKS;
}

inline uint8_t md_length(const void *track) { return md(track)->length; }
inline uint8_t md_speed(const void *track) { return md(track)->speed; }
inline uint8_t md_step_count(const void *track) {
  return md(track)->step_count;
}
inline uint8_t md_track_index(const void *track) {
  return md(track)->track_number;
}
inline uint8_t md_mute_state(const void *track) {
  return md(track)->mute_state;
}
inline void md_set_mute_state(void *track, uint8_t state) {
  md(track)->mute_state = state;
}
inline void md_set_length(void *track, uint8_t len, bool expand) {
  md(track)->set_length(len, expand);
}
inline bool md_request_speed_change(void *track, uint8_t new_speed) {
  return md(track)->request_speed_change(new_speed);
}
inline uint8_t md_condition_count(const void *track) {
  (void)track;
  return NUM_TRIG_CONDITIONS;
}
inline uint8_t md_timing_encoder_min(const void *track) {
  (void)track;
  return 1;
}
inline uint8_t md_timing_mid(const MDSeqTrack *track) {
  return SeqTrack::get_timing_mid(track->speed);
}
inline uint8_t md_timing_encoder_center(const void *track) {
  return md_timing_mid(md(track));
}
inline uint8_t md_timing_encoder_max(const void *track) {
  return md_timing_mid(md(track)) * 2 - 1;
}
inline uint8_t md_timing_display_mid(const void *track) {
  return md_timing_mid(md(track));
}
inline uint8_t md_timing_encoder_for_step(const void *track, uint8_t step) {
  const MDSeqTrack *t = md(track);
  uint8_t timing = t->timing[step];
  return timing == 0 ? md_timing_mid(t) : timing;
}
inline int8_t md_microtiming_for_step(const void *track, uint8_t step) {
  (void)track;
  (void)step;
  return 0;
}
inline void md_rotate_left(void *track) { md(track)->rotate_left(); }
inline void md_rotate_right(void *track) { md(track)->rotate_right(); }
inline void md_reverse(void *track) { md(track)->reverse(); }
inline void md_transpose(void *track, int8_t offset) {
  md(track)->transpose(offset);
}
inline void md_get_mask(const void *track, uint64_t *mask, uint8_t ui_mask) {
  md(track)->get_mask(mask, ui_mask);
}
inline bool md_get_step(const void *track, uint8_t step, uint8_t ui_mask) {
  return md(track)->get_step(step, ui_mask);
}
inline void md_set_step(void *track, uint8_t step, uint8_t ui_mask,
                        bool value) {
  md(track)->set_step(step, ui_mask, value);
}
inline uint8_t md_conditional_id(const void *track, uint8_t step) {
  return md(track)->steps[step].cond_id;
}
inline bool md_conditional_plock(const void *track, uint8_t step) {
  return md(track)->steps[step].cond_plock;
}
inline void md_set_conditional(void *track, uint8_t step, uint8_t condition,
                               bool plock) {
  md(track)->steps[step].cond_id = condition;
  md(track)->steps[step].cond_plock = plock;
}
inline void md_set_timing_from_encoder(void *track, uint8_t step,
                                       uint8_t encoder_value) {
  md(track)->timing[step] = encoder_value;
}
inline void md_set_pattern_step_from_edit(void *track, uint8_t step,
                                          uint8_t condition, bool cond_plock,
                                          uint8_t timing_encoder) {
  MDSeqTrack *t = md(track);
  t->steps[step].trig = true;
  t->steps[step].cond_id = condition;
  t->steps[step].cond_plock = cond_plock;
  t->timing[step] = timing_encoder;
}
inline void md_reset_timing(void *track, uint8_t step) {
  MDSeqTrack *t = md(track);
  t->timing[step] = t->get_timing_mid();
}
inline void md_clear_mute(void *track, uint8_t step) {
  md(track)->mute_mask &= ~(1ULL << step);
}
inline void md_toggle_mute(void *track, uint8_t step) {
  md(track)->mute_mask ^= (1ULL << step);
}
inline uint64_t md_mute_mask(const void *track) { return md(track)->mute_mask; }
inline void md_enable_step_locks(void *track, uint8_t step) {
  md(track)->enable_step_locks(step);
}
inline void md_clear_step_lock(void *track, uint8_t step, uint8_t param_id) {
  md(track)->clear_step_lock(step, param_id);
}
inline void md_clear_step_locks(void *track, uint8_t step) {
  md(track)->clear_step_locks(step);
}
inline void md_clear_param_locks(void *track, uint8_t param_id) {
  md(track)->clear_param_locks(param_id);
}
inline void md_clear_locks(void *track) { md(track)->clear_locks(); }
inline bool md_set_track_locks(void *track, uint8_t step, uint8_t param_id,
                               uint8_t value) {
  return md(track)->set_track_locks(step, param_id, value);
}
inline bool md_step_has_lock(const void *track, uint8_t step,
                             uint8_t lock_idx) {
  return md(track)->steps[step].is_lock(lock_idx);
}
inline uint8_t md_find_param(const void *track, uint8_t param_id) {
  return md(track)->find_param(param_id);
}
inline uint8_t md_get_track_lock_implicit(void *track, uint8_t step,
                                          uint8_t param_id) {
  return md(track)->get_track_lock_implicit(step, param_id);
}
inline void md_clear_track(void *track, bool locks) {
  md(track)->clear_track(locks);
}
inline void md_clear_step(void *track, uint8_t step) {
  MDSeqStep empty_step;
  memset(&empty_step, 0, sizeof(empty_step));
  md(track)->paste_step(step, &empty_step);
}
inline void md_clean_params(void *track) { md(track)->clean_params(); }
inline void md_copy_step(void *track, uint8_t step, MDSeqStep *md_step,
                         void *stepseq_step) {
  (void)stepseq_step;
  md(track)->copy_step(step, md_step);
}
inline void md_paste_step(void *track, uint8_t step, MDSeqStep *md_step,
                          void *stepseq_step) {
  (void)stepseq_step;
  md(track)->paste_step(step, md_step);
}
inline void md_set_track_pitch(void *track, uint8_t step, uint8_t pitch) {
  md(track)->set_track_pitch(step, pitch);
}
inline void md_get_step_locks(void *track, uint8_t step, uint8_t *params,
                              bool ignore_locks_disabled) {
  md(track)->get_step_locks(step, params, ignore_locks_disabled);
}
inline void md_record_track(void *track, uint8_t velocity) {
  md(track)->record_track(velocity);
}
inline void md_record_track_locks(void *track, uint8_t param_id,
                                  uint8_t value) {
  md(track)->record_track_locks(param_id, value);
}
inline bool md_preview_step(void *track, uint8_t step) {
  MDSeqTrack *t = md(track);
  t->send_parameter_locks(step, true);
  MD.triggerTrack(t->track_number, 127);
  return true;
}

#if !defined(__AVR__)
inline StepSeqDataTrack *stepseq(void *track) {
  return static_cast<StepSeqDataTrack *>(track);
}

inline const StepSeqDataTrack *stepseq(const void *track) {
  return static_cast<const StepSeqDataTrack *>(track);
}

inline uint8_t stepseq_mask_for(uint8_t ui_mask) {
  switch (ui_mask) {
  case MASK_LOCK:
    return STEPSEQ_MASK_LOCK;
  case MASK_MUTE:
    return STEPSEQ_MASK_MUTE;
  case MASK_SLIDE:
    return STEPSEQ_MASK_SLIDE;
  case MASK_LOCKS_ON_STEP:
    return STEPSEQ_MASK_LOCKS_ON_STEP;
  case MASK_PATTERN:
  default:
    return STEPSEQ_MASK_PATTERN;
  }
}

inline bool stepseq_uses_kit_sound(const void *track) {
  return !stepseq(track)->owns_sound_data();
}

inline bool stepseq_selects_track_locally(const void *track) {
  return stepseq(track)->owns_sound_data();
}

inline uint8_t stepseq_lock_slot_count(const void *track) {
  (void)track;
  return STEPSEQ_NUM_LOCKS;
}
inline uint8_t stepseq_length(const void *track) {
  return stepseq(track)->length;
}
inline uint8_t stepseq_speed(const void *track) {
  return stepseq(track)->speed;
}
inline uint8_t stepseq_step_count(const void *track) {
  return stepseq(track)->step_count;
}
inline uint8_t stepseq_track_index(const void *track) {
  return stepseq(track)->track_number;
}
inline uint8_t stepseq_mute_state(const void *track) {
  return stepseq(track)->mute_state;
}
inline void stepseq_set_mute_state(void *track, uint8_t state) {
  stepseq(track)->mute_state = state;
}
inline void stepseq_set_length(void *track, uint8_t len, bool expand) {
  stepseq(track)->set_length(len, expand);
}
inline bool stepseq_request_speed_change(void *track, uint8_t new_speed) {
  StepSeqDataTrack *t = stepseq(track);
  if (t->count_down || t->speed == new_speed) {
    return false;
  }
  t->set_speed(new_speed, t->speed, true);
  return true;
}
inline uint8_t stepseq_condition_count(const void *track) {
  (void)track;
  return STEPSEQ_NUM_TRIG_CONDITIONS - 1;
}
inline uint8_t stepseq_timing_encoder_min(const void *track) {
  (void)track;
  return 0;
}
inline uint8_t stepseq_timing_encoder_center(const void *track) {
  (void)track;
  return 127;
}
inline uint8_t stepseq_timing_encoder_max(const void *track) {
  (void)track;
  return 254;
}
inline uint8_t stepseq_timing_display_mid(const void *track) {
  (void)track;
  return 0;
}
inline uint8_t stepseq_timing_encoder_for_step(const void *track,
                                               uint8_t step) {
  return (uint8_t)(stepseq(track)->microtiming[step] + 127);
}
inline int8_t stepseq_microtiming_for_step(const void *track, uint8_t step) {
  return stepseq(track)->microtiming[step];
}
inline void stepseq_rotate_left(void *track) { stepseq(track)->rotate_left(); }
inline void stepseq_rotate_right(void *track) {
  stepseq(track)->rotate_right();
}
inline void stepseq_reverse(void *track) { stepseq(track)->reverse(); }
inline void stepseq_transpose(void *track, int8_t offset) {
  stepseq(track)->transpose(offset);
}
inline void stepseq_get_mask(const void *track, uint64_t *mask,
                             uint8_t ui_mask) {
  stepseq(track)->get_mask(mask, stepseq_mask_for(ui_mask));
}
inline bool stepseq_get_step(const void *track, uint8_t step, uint8_t ui_mask) {
  return stepseq(track)->get_step(step, stepseq_mask_for(ui_mask));
}
inline void stepseq_set_step(void *track, uint8_t step, uint8_t ui_mask,
                             bool value) {
  stepseq(track)->set_step(step, stepseq_mask_for(ui_mask), value);
}
inline uint8_t stepseq_conditional_id(const void *track, uint8_t step) {
  return stepseq(track)->steps[step].cond_id;
}
inline bool stepseq_conditional_plock(const void *track, uint8_t step) {
  return stepseq(track)->steps[step].cond_plock;
}
inline void stepseq_set_conditional(void *track, uint8_t step,
                                    uint8_t condition, bool plock) {
  StepSeqDataTrack *t = stepseq(track);
  t->steps[step].cond_id = condition;
  t->steps[step].cond_plock = plock;
}
inline void stepseq_set_timing_from_encoder(void *track, uint8_t step,
                                            uint8_t encoder_value) {
  stepseq(track)->microtiming[step] = (int8_t)((int16_t)encoder_value - 127);
}
inline void stepseq_set_pattern_step_from_edit(void *track, uint8_t step,
                                               uint8_t condition,
                                               bool cond_plock,
                                               uint8_t timing_encoder) {
  StepSeqDataTrack *t = stepseq(track);
  t->set_step(step, STEPSEQ_MASK_PATTERN, true);
  t->steps[step].cond_id = condition;
  t->steps[step].cond_plock = cond_plock;
  t->microtiming[step] = (int8_t)((int16_t)timing_encoder - 127);
}
inline void stepseq_reset_timing(void *track, uint8_t step) {
  stepseq(track)->microtiming[step] = 0;
}
inline void stepseq_clear_mute(void *track, uint8_t step) {
  stepseq(track)->mute_mask &= ~(1ULL << step);
}
inline void stepseq_toggle_mute(void *track, uint8_t step) {
  stepseq(track)->mute_mask ^= (1ULL << step);
}
inline uint64_t stepseq_mute_mask(const void *track) {
  return stepseq(track)->mute_mask;
}
inline void stepseq_enable_step_locks(void *track, uint8_t step) {
  stepseq(track)->enable_step_locks(step);
}
inline void stepseq_clear_step_lock(void *track, uint8_t step,
                                    uint8_t param_id) {
  stepseq(track)->clear_step_lock(step, param_id);
}
inline void stepseq_clear_step_locks(void *track, uint8_t step) {
  stepseq(track)->clear_step_locks(step);
}
inline void stepseq_clear_param_locks(void *track, uint8_t param_id) {
  stepseq(track)->clear_param_locks(param_id);
}
inline void stepseq_clear_locks(void *track) { stepseq(track)->clear_locks(); }
inline bool stepseq_set_track_locks(void *track, uint8_t step, uint8_t param_id,
                                    uint8_t value) {
  return stepseq(track)->set_track_locks(step, param_id, value);
}
inline bool stepseq_step_has_lock(const void *track, uint8_t step,
                                  uint8_t lock_idx) {
  return stepseq(track)->steps[step].is_lock(lock_idx);
}
inline uint8_t stepseq_find_param(const void *track, uint8_t param_id) {
  return stepseq(track)->find_param(param_id);
}
inline uint8_t stepseq_get_track_lock_implicit(void *track, uint8_t step,
                                               uint8_t param_id) {
  return stepseq(track)->get_track_lock_implicit(step, param_id);
}
inline void stepseq_clear_track(void *track, bool locks) {
  stepseq(track)->clear_track(locks);
}
inline void stepseq_clear_step(void *track, uint8_t step) {
  stepseq(track)->clear_step(step);
}
inline void stepseq_clean_params(void *track) {
  stepseq(track)->clean_params();
}
inline void stepseq_copy_step(void *track, uint8_t step, MDSeqStep *md_step,
                              void *stepseq_step) {
  (void)md_step;
  stepseq(track)->copy_step(step, static_cast<StepSeqStep *>(stepseq_step));
}
inline void stepseq_paste_step(void *track, uint8_t step, MDSeqStep *md_step,
                               void *stepseq_step) {
  (void)md_step;
  stepseq(track)->paste_step(step, static_cast<StepSeqStep *>(stepseq_step));
}
inline void stepseq_set_track_pitch(void *track, uint8_t step, uint8_t pitch) {
  stepseq(track)->set_track_pitch(step, pitch);
}
inline void stepseq_get_step_locks(void *track, uint8_t step, uint8_t *params,
                                   bool ignore_locks_disabled) {
  stepseq(track)->get_step_locks(step, params, ignore_locks_disabled);
}
inline void stepseq_record_track(void *track, uint8_t velocity) {
  stepseq(track)->record_track(velocity);
}
inline void stepseq_record_track_locks(void *track, uint8_t param_id,
                                       uint8_t value) {
  stepseq(track)->record_track_locks(param_id, value);
}
inline bool stepseq_preview_step(void *track, uint8_t step) {
  return stepseq(track)->preview_step(step);
}
#endif

} // namespace SeqStepTrackBackend

const SeqStepTrackOps *seq_step_md_ops() {
  using namespace SeqStepTrackBackend;
  static const SeqStepTrackOps ops = {
      false,
      false,
      true,
      false,
      md_uses_kit_sound,
      md_selects_track_locally,
      md_lock_slot_count,
      md_length,
      md_speed,
      md_step_count,
      md_track_index,
      md_mute_state,
      md_set_mute_state,
      md_set_length,
      md_request_speed_change,
      md_condition_count,
      md_timing_encoder_min,
      md_timing_encoder_center,
      md_timing_encoder_max,
      md_timing_display_mid,
      md_timing_encoder_for_step,
      md_microtiming_for_step,
      md_rotate_left,
      md_rotate_right,
      md_reverse,
      md_transpose,
      md_get_mask,
      md_get_step,
      md_set_step,
      md_conditional_id,
      md_conditional_plock,
      md_set_conditional,
      md_set_timing_from_encoder,
      md_set_pattern_step_from_edit,
      md_reset_timing,
      md_clear_mute,
      md_toggle_mute,
      md_mute_mask,
      md_enable_step_locks,
      md_clear_step_lock,
      md_clear_step_locks,
      md_clear_param_locks,
      md_clear_locks,
      md_set_track_locks,
      md_step_has_lock,
      md_find_param,
      md_get_track_lock_implicit,
      md_clear_track,
      md_clear_step,
      md_clean_params,
      md_copy_step,
      md_paste_step,
      md_set_track_pitch,
      md_get_step_locks,
      md_record_track,
      md_record_track_locks,
      md_preview_step,
  };
  return &ops;
}

#if !defined(__AVR__)
const SeqStepTrackOps *seq_step_stepseq_ops() {
  using namespace SeqStepTrackBackend;
  static const SeqStepTrackOps ops = {
      true,
      true,
      false,
      true,
      stepseq_uses_kit_sound,
      stepseq_selects_track_locally,
      stepseq_lock_slot_count,
      stepseq_length,
      stepseq_speed,
      stepseq_step_count,
      stepseq_track_index,
      stepseq_mute_state,
      stepseq_set_mute_state,
      stepseq_set_length,
      stepseq_request_speed_change,
      stepseq_condition_count,
      stepseq_timing_encoder_min,
      stepseq_timing_encoder_center,
      stepseq_timing_encoder_max,
      stepseq_timing_display_mid,
      stepseq_timing_encoder_for_step,
      stepseq_microtiming_for_step,
      stepseq_rotate_left,
      stepseq_rotate_right,
      stepseq_reverse,
      stepseq_transpose,
      stepseq_get_mask,
      stepseq_get_step,
      stepseq_set_step,
      stepseq_conditional_id,
      stepseq_conditional_plock,
      stepseq_set_conditional,
      stepseq_set_timing_from_encoder,
      stepseq_set_pattern_step_from_edit,
      stepseq_reset_timing,
      stepseq_clear_mute,
      stepseq_toggle_mute,
      stepseq_mute_mask,
      stepseq_enable_step_locks,
      stepseq_clear_step_lock,
      stepseq_clear_step_locks,
      stepseq_clear_param_locks,
      stepseq_clear_locks,
      stepseq_set_track_locks,
      stepseq_step_has_lock,
      stepseq_find_param,
      stepseq_get_track_lock_implicit,
      stepseq_clear_track,
      stepseq_clear_step,
      stepseq_clean_params,
      stepseq_copy_step,
      stepseq_paste_step,
      stepseq_set_track_pitch,
      stepseq_get_step_locks,
      stepseq_record_track,
      stepseq_record_track_locks,
      stepseq_preview_step,
  };
  return &ops;
}
#endif
