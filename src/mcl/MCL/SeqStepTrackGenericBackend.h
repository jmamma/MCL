#pragma once

#include "../Drivers/MidiDevice.h"
#include "DeviceParamResolver.h"
#include "SeqDefines.h"
#include <stddef.h>
#include <stdint.h>

struct SeqStepTrackOps {
  bool uses_signed_microtiming;
  bool clears_mute_on_pattern_clear;
  bool shows_lock_value_popup;
  bool (*uses_kit_sound)(const void *track);
  bool (*selects_track_locally)(const void *track);
  uint8_t (*lock_slot_count)(const void *track);
  uint8_t (*length)(const void *track);
  uint8_t (*speed)(const void *track);
  uint8_t (*step_count)(const void *track);
  uint8_t (*track_index)(const void *track);
  uint8_t (*mute_state)(const void *track);
  void (*set_mute_state)(void *track, uint8_t state);
  void (*set_length)(void *track, uint8_t len, bool expand);
  bool (*request_speed_change)(void *track, uint8_t new_speed);
  uint8_t (*condition_count)(const void *track);
  void (*condition_label)(const void *track, uint8_t condition, bool plock,
                          bool marker, char *out);
  uint8_t (*timing_encoder_min)(const void *track);
  uint8_t (*timing_encoder_center)(const void *track);
  uint8_t (*timing_encoder_max)(const void *track);
  uint8_t (*timing_display_mid)(const void *track);
  uint8_t (*timing_encoder_for_step)(const void *track, uint8_t step);
  int8_t (*microtiming_for_step)(const void *track, uint8_t step);
  void (*rotate_left)(void *track);
  void (*rotate_right)(void *track);
  void (*reverse)(void *track);
  void (*transpose)(void *track, int8_t offset);
  void (*get_mask)(const void *track, uint64_t *mask, uint8_t ui_mask);
  bool (*get_step)(const void *track, uint8_t step, uint8_t ui_mask);
  void (*set_step)(void *track, uint8_t step, uint8_t ui_mask, bool value);
  uint8_t (*conditional_id)(const void *track, uint8_t step);
  bool (*conditional_plock)(const void *track, uint8_t step);
  void (*set_conditional)(void *track, uint8_t step, uint8_t condition,
                          bool plock);
  void (*set_timing_from_encoder)(void *track, uint8_t step,
                                  uint8_t encoder_value);
  void (*set_pattern_step_from_edit)(void *track, uint8_t step,
                                     uint8_t condition, bool cond_plock,
                                     uint8_t timing_encoder);
  void (*reset_timing)(void *track, uint8_t step);
  void (*clear_mute)(void *track, uint8_t step);
  void (*toggle_mute)(void *track, uint8_t step);
  uint64_t (*mute_mask)(const void *track);
  void (*enable_step_locks)(void *track, uint8_t step);
  void (*clear_step_lock)(void *track, uint8_t step, uint8_t param_id);
  void (*clear_step_locks)(void *track, uint8_t step);
  void (*clear_param_locks)(void *track, uint8_t param_id);
  void (*clear_locks)(void *track);
  bool (*set_track_locks)(void *track, uint8_t step, uint8_t param_id,
                          uint8_t value);
  bool (*step_has_lock)(const void *track, uint8_t step, uint8_t lock_idx);
  uint8_t (*find_param)(const void *track, uint8_t param_id);
  uint8_t (*get_track_lock_implicit)(void *track, uint8_t step,
                                     uint8_t param_id);
  void (*clear_track)(void *track, bool locks);
  void (*clear_step)(void *track, uint8_t step);
  void (*clean_params)(void *track);
  void (*copy_step)(void *track, uint8_t step, MDSeqStep *md_step,
                    void *stepseq_step);
  void (*paste_step)(void *track, uint8_t step, MDSeqStep *md_step,
                     void *stepseq_step);
  void (*set_track_pitch)(void *track, uint8_t step, uint8_t pitch);
  void (*get_step_locks)(void *track, uint8_t step, uint8_t *params,
                         bool ignore_locks_disabled);
  void (*record_track)(void *track, uint8_t velocity);
  void (*record_track_locks)(void *track, uint8_t param_id, uint8_t value);
  bool (*preview_step)(void *track, uint8_t step);
  void (*set_linked_param_update)(void *track, bool enabled);
};

const SeqStepTrackOps *seq_step_md_ops();
const SeqStepTrackOps *seq_step_stepseq_ops();

class SeqStepTrackGenericBackend {
public:
  explicit SeqStepTrackGenericBackend(MDSeqTrack &track,
                                      uint8_t device_slot = 1)
      : track_(&track), ops_(seq_step_md_ops()), device_slot_(device_slot) {}

  explicit SeqStepTrackGenericBackend(StepSeqDataTrack &track,
                                      uint8_t device_slot = 1)
      : track_(&track), ops_(seq_step_stepseq_ops()),
        device_slot_(device_slot) {}

  bool uses_signed_microtiming() const { return ops_->uses_signed_microtiming; }
  bool clears_mute_on_pattern_clear() const {
    return ops_->clears_mute_on_pattern_clear;
  }
  bool shows_lock_value_popup() const { return ops_->shows_lock_value_popup; }
  bool uses_kit_sound() const { return ops_->uses_kit_sound(track_); }
  bool selects_track_locally() const {
    return ops_->selects_track_locally(track_);
  }
  bool uses_step_pitch() const {
    return uses_kit_sound() ||
           DeviceParamResolver::slot(param_device_slot(), param_dest())
               .uses_step_pitch();
  }

  bool configure_panel(char *info, size_t info_len, uint8_t &pitch_encoder_max,
                       bool &is_midi_model) const {
    if (uses_kit_sound()) {
      return step_edit()->configure_kit_sound_panel(
          param_device_idx(), param_target(), info, (uint8_t)info_len,
          &pitch_encoder_max, &is_midi_model);
    }
    pitch_encoder_max = uses_step_pitch() ? 127 : 1;
    is_midi_model = false;
    return DeviceParamResolver::slot(param_device_slot(), param_dest())
        .target_label(info, (uint8_t)info_len);
  }

  bool step_editor_available() const {
    return !uses_kit_sound() || step_edit()->available(param_device_idx());
  }

  void set_step_edit_rec_mode(uint8_t mode) const {
    if (uses_kit_sound()) {
      step_edit()->set_rec_mode(param_device_idx(), mode);
    }
  }

  void sync_step_edit() const {
    if (uses_kit_sound()) {
      step_edit()->sync_track(param_device_idx(), length(), speed(),
                              step_count());
    }
  }

  void set_step_edit_trig_leds(uint16_t mask, uint8_t mode,
                               uint8_t blink = 0) const {
    if (uses_kit_sound()) {
      step_edit()->set_trig_leds(param_device_idx(), mask, mode, blink);
    }
  }

  void set_live_param_update(bool enabled) const {
    if (uses_kit_sound()) {
      step_edit()->set_live_param_update(param_device_idx(), enabled);
      ops_->set_linked_param_update(track_, enabled);
    }
  }

  bool uses_note_pitch() const {
    return uses_kit_sound() ? step_edit()->kit_sound_uses_note_pitch(
                                  param_device_idx(), param_target())
                            : uses_step_pitch();
  }
  uint8_t note_from_pitch_lock(uint8_t pitch) const {
    return uses_kit_sound() ? step_edit()->kit_sound_note_from_pitch(
                                  param_device_idx(), param_target(), pitch)
                            : pitch;
  }
  uint8_t pitch_lock_from_note(uint8_t note, uint8_t fine_tune = 255) const {
    return uses_kit_sound()
               ? step_edit()->kit_sound_pitch_from_note(
                     param_device_idx(), param_target(), note, fine_tune)
               : note;
  }
  uint8_t default_pitch_lock() const {
    return uses_kit_sound() ? step_edit()->kit_sound_default_pitch(
                                  param_device_idx(), param_target())
                            : 0;
  }

  bool param_from_key(uint8_t key, uint8_t *param) const {
    return uses_kit_sound() &&
           step_edit()->param_from_key(param_device_idx(), param_target(), key,
                                       param);
  }
  bool key_for_param(uint8_t param, uint8_t *key) const {
    return uses_kit_sound() &&
           step_edit()->key_for_param(param_device_idx(), param_target(), param,
                                      key);
  }
  bool begin_param_editor(uint8_t *params, uint8_t count) const {
    return uses_kit_sound() &&
           step_edit()->begin_param_editor(param_device_idx(), param_target(),
                                           params, count);
  }
  void end_param_editor() const {
    if (uses_kit_sound()) {
      step_edit()->end_param_editor(param_device_idx());
    }
  }
  void close_microtiming() const {
    if (uses_kit_sound()) {
      step_edit()->close_microtiming(param_device_idx());
    }
  }
  void clear_step_edit_popup() const {
    if (uses_kit_sound()) {
      step_edit()->clear_popup(param_device_idx());
    }
  }
  void popup_text(char *text, uint8_t persistent = 0) const {
    if (uses_kit_sound()) {
      step_edit()->popup_text(param_device_idx(), text, persistent);
    }
  }

  uint8_t lock_param_count() const {
    return DeviceParamResolver::slot(param_device_slot(), param_dest())
        .lock_param_count();
  }
  uint8_t lock_slot_count() const { return ops_->lock_slot_count(track_); }
  bool lock_param_info(uint8_t param_id, SeqStepLockParamInfo &info) const {
    return DeviceParamResolver::slot(param_device_slot(), param_dest())
        .lock_param_info(param_id, &info);
  }
  uint8_t current_lock_value(uint8_t param_id) const {
    uint8_t value = 0;
    DeviceParamResolver::slot(param_device_slot(), param_dest())
        .lock_current_value(param_id, &value);
    return value;
  }
  bool copy_lock_param_label(uint8_t param_id, char *dst,
                             size_t dst_len) const {
    return DeviceParamResolver::slot(param_device_slot(), param_dest())
        .lock_param_label(param_id, dst, (uint8_t)dst_len);
  }

  uint8_t length() const { return ops_->length(track_); }
  uint8_t speed() const { return ops_->speed(track_); }
  uint8_t step_count() const { return ops_->step_count(track_); }
  uint8_t track_index() const { return ops_->track_index(track_); }
  uint8_t mute_state() const { return ops_->mute_state(track_); }
  void set_mute_state(uint8_t state) { ops_->set_mute_state(track_, state); }
  void set_length(uint8_t len, bool expand = false) {
    ops_->set_length(track_, len, expand);
  }
  bool request_speed_change(uint8_t new_speed) {
    return ops_->request_speed_change(track_, new_speed);
  }

  uint8_t condition_count() const { return ops_->condition_count(track_); }
  void condition_label(uint8_t condition, bool plock, bool marker,
                       char *out) const {
    ops_->condition_label(track_, condition, plock, marker, out);
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
    return ops_->timing_encoder_min(track_);
  }
  uint8_t timing_encoder_center() const {
    return ops_->timing_encoder_center(track_);
  }
  uint8_t timing_encoder_max() const {
    return ops_->timing_encoder_max(track_);
  }
  uint8_t timing_display_mid() const {
    return ops_->timing_display_mid(track_);
  }
  uint8_t timing_encoder_for_step(uint8_t step) const {
    return ops_->timing_encoder_for_step(track_, step);
  }
  int8_t microtiming_from_encoder(uint8_t encoder_value) const {
    return (int8_t)((int16_t)encoder_value - 127);
  }
  int8_t microtiming_for_step(uint8_t step) const {
    return ops_->microtiming_for_step(track_, step);
  }

  void rotate_left() { ops_->rotate_left(track_); }
  void rotate_right() { ops_->rotate_right(track_); }
  void reverse() { ops_->reverse(track_); }
  void transpose(int8_t offset) { ops_->transpose(track_, offset); }
  void get_mask(uint64_t *mask, uint8_t ui_mask) const {
    ops_->get_mask(track_, mask, ui_mask);
  }
  bool get_step(uint8_t step, uint8_t ui_mask) const {
    return ops_->get_step(track_, step, ui_mask);
  }
  void set_step(uint8_t step, uint8_t ui_mask, bool value) {
    ops_->set_step(track_, step, ui_mask, value);
  }

  uint8_t conditional_id(uint8_t step) const {
    return ops_->conditional_id(track_, step);
  }
  bool conditional_plock(uint8_t step) const {
    return ops_->conditional_plock(track_, step);
  }
  void set_conditional(uint8_t step, uint8_t condition, bool plock) {
    ops_->set_conditional(track_, step, condition, plock);
  }
  void clear_conditional(uint8_t step) { set_conditional(step, 0, false); }
  void set_timing_from_encoder(uint8_t step, uint8_t encoder_value) {
    ops_->set_timing_from_encoder(track_, step, encoder_value);
  }
  void set_pattern_step_from_edit(uint8_t step, uint8_t condition_knob,
                                  uint8_t timing_encoder) {
    bool cond_plock;
    uint8_t condition = step_conditional_from_knob(condition_knob, &cond_plock);
    ops_->set_pattern_step_from_edit(track_, step, condition, cond_plock,
                                     timing_encoder);
  }
  void reset_timing(uint8_t step) { ops_->reset_timing(track_, step); }
  void clear_mute(uint8_t step) { ops_->clear_mute(track_, step); }
  void toggle_mute(uint8_t step) { ops_->toggle_mute(track_, step); }
  uint64_t mute_mask() const { return ops_->mute_mask(track_); }

  void enable_step_locks(uint8_t step) {
    ops_->enable_step_locks(track_, step);
  }
  void clear_step_lock(uint8_t step, uint8_t param_id) {
    ops_->clear_step_lock(track_, step, param_id);
  }
  void clear_step_locks(uint8_t step) { ops_->clear_step_locks(track_, step); }
  void clear_param_locks(uint8_t param_id) {
    ops_->clear_param_locks(track_, param_id);
  }
  void clear_locks() { ops_->clear_locks(track_); }
  bool set_track_locks(uint8_t step, uint8_t param_id, uint8_t value) {
    return ops_->set_track_locks(track_, step, param_id, value);
  }
  bool step_has_lock(uint8_t step, uint8_t lock_idx) const {
    return ops_->step_has_lock(track_, step, lock_idx);
  }
  int8_t find_param(uint8_t param_id) const {
    uint8_t lock_idx = ops_->find_param(track_, param_id);
    return lock_idx == 255 ? -1 : (int8_t)lock_idx;
  }
  uint8_t pitch_lock_param_id() const {
    return DeviceParamResolver::slot(param_device_slot(), param_dest())
        .pitch_lock_param();
  }
  uint8_t get_track_lock_implicit(uint8_t step, uint8_t param_id) {
    return ops_->get_track_lock_implicit(track_, step, param_id);
  }
  void clear_track(bool locks = true) { ops_->clear_track(track_, locks); }
  void clear_step(uint8_t step) { ops_->clear_step(track_, step); }
  void clean_params() { ops_->clean_params(track_); }
  void copy_step(uint8_t step, MDSeqStep *md_step,
                 StepSeqStep *stepseq_step) {
    ops_->copy_step(track_, step, md_step, stepseq_step);
  }
  void paste_step(uint8_t step, MDSeqStep *md_step,
                  StepSeqStep *stepseq_step) {
    ops_->paste_step(track_, step, md_step, stepseq_step);
  }
  void set_track_pitch(uint8_t step, uint8_t pitch) {
    ops_->set_track_pitch(track_, step, pitch);
  }
  void get_step_locks(uint8_t step, uint8_t *params,
                      bool ignore_locks_disabled) {
    ops_->get_step_locks(track_, step, params, ignore_locks_disabled);
  }
  void record_track(uint8_t velocity) { ops_->record_track(track_, velocity); }
  void record_track_locks(uint8_t param_id, uint8_t value) {
    ops_->record_track_locks(track_, param_id, value);
  }
  bool preview_step(uint8_t step) { return ops_->preview_step(track_, step); }

private:
  uint8_t param_device_slot() const { return device_slot_; }
  uint8_t param_dest() const { return track_index() + 1; }
  uint8_t param_device_idx() const {
    return DeviceParamResolver::slot_device_idx(param_device_slot());
  }
  uint8_t param_target() const { return track_index(); }
  DeviceStepEditCapability *step_edit() const {
    return DeviceParamResolver::slot_device(param_device_slot())->step_edit();
  }

  void *track_;
  const SeqStepTrackOps *ops_;
  uint8_t device_slot_;
};
