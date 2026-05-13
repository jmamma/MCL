#pragma once

#include "../Drivers/Generic/Sequencer/StepSeqDefines.h"
#include "../Drivers/Generic/Sequencer/StepSeqTrack.h"
#include "../Drivers/MD/MD.h"
#include "../Drivers/MD/Sequencer/MDSeqTrack.h"
#include "../Drivers/MidiDevice.h"
#include "DeviceManager.h"
#include "DeviceParamResolver.h"
#include "SeqDefines.h"
#include "SeqTrack.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

class SeqStepTrackGenericBackend {
public:
  explicit SeqStepTrackGenericBackend(MDSeqTrack &track,
                                      uint8_t device_slot = 1)
      : kind_(KIND_MD), device_slot_(device_slot) {
    tracks_.md = &track;
  }

  explicit SeqStepTrackGenericBackend(StepSeqDataTrack &track,
                                      uint8_t device_slot = 1)
      : kind_(KIND_STEPSEQ), device_slot_(device_slot) {
    tracks_.stepseq = &track;
  }

  bool uses_signed_microtiming() const { return kind_ == KIND_STEPSEQ; }
  bool clears_mute_on_pattern_clear() const { return kind_ == KIND_STEPSEQ; }
  bool shows_lock_value_popup() const { return kind_ == KIND_MD; }

  bool uses_kit_sound() const {
    return kind_ == KIND_MD ? true : !tracks_.stepseq->owns_sound_data();
  }
  bool selects_track_locally() const {
    return kind_ == KIND_STEPSEQ && tracks_.stepseq->owns_sound_data();
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
          param_context(), param_target(), info, (uint8_t)info_len,
          &pitch_encoder_max, &is_midi_model);
    }
    pitch_encoder_max = uses_step_pitch() ? 127 : 1;
    is_midi_model = false;
    return DeviceParamResolver::slot(param_device_slot(), param_dest())
        .target_label(info, (uint8_t)info_len);
  }

  bool step_editor_available() const {
    return !uses_kit_sound() || step_edit()->available(param_context());
  }

  void set_step_edit_rec_mode(uint8_t mode) const {
    if (uses_kit_sound()) {
      step_edit()->set_rec_mode(param_context(), mode);
    }
  }

  void sync_step_edit() const {
    if (uses_kit_sound()) {
      step_edit()->sync_track(param_context(), length(), speed(),
                              step_count());
    }
  }

  void sync_step_edit(uint8_t length, uint8_t speed, uint8_t step_count) const {
    if (uses_kit_sound()) {
      step_edit()->sync_track(param_context(), length, speed, step_count);
    }
  }

  void set_step_edit_trig_leds(uint16_t mask, uint8_t mode,
                               uint8_t blink = 0) const {
    if (uses_kit_sound()) {
      step_edit()->set_trig_leds(param_context(), mask, mode, blink);
    }
  }

  void set_live_param_update(bool enabled) const;

  bool uses_note_pitch() const {
    return uses_kit_sound() ? step_edit()->kit_sound_uses_note_pitch(
                                  param_context(), param_target())
                            : uses_step_pitch();
  }
  uint8_t note_from_pitch_lock(uint8_t pitch) const {
    return uses_kit_sound() ? step_edit()->kit_sound_note_from_pitch(
                                  param_context(), param_target(), pitch)
                            : pitch;
  }
  uint8_t pitch_lock_from_note(uint8_t note, uint8_t fine_tune = 255) const {
    return uses_kit_sound()
               ? step_edit()->kit_sound_pitch_from_note(
                     param_context(), param_target(), note, fine_tune)
               : note;
  }
  uint8_t default_pitch_lock() const {
    return uses_kit_sound() ? step_edit()->kit_sound_default_pitch(
                                  param_context(), param_target())
                            : 0;
  }

  bool param_from_key(uint8_t key, uint8_t *param) const {
    return uses_kit_sound() &&
           step_edit()->param_from_key(param_context(), param_target(), key,
                                       param);
  }
  bool key_for_param(uint8_t param, uint8_t *key) const {
    return uses_kit_sound() &&
           step_edit()->key_for_param(param_context(), param_target(), param,
                                      key);
  }
  bool begin_param_editor(uint8_t *params, uint8_t count) const {
    return uses_kit_sound() &&
           step_edit()->begin_param_editor(param_context(), param_target(),
                                           params, count);
  }
  void end_param_editor() const {
    if (uses_kit_sound()) {
      step_edit()->end_param_editor(param_context());
    }
  }
  void close_microtiming() const {
    if (uses_kit_sound()) {
      step_edit()->close_microtiming(param_context());
    }
  }
  void clear_step_edit_popup() const {
    if (uses_kit_sound()) {
      step_edit()->clear_popup(param_context());
    }
  }
  void popup_text(char *text, uint8_t persistent = 0) const {
    if (uses_kit_sound()) {
      step_edit()->popup_text(param_context(), text, persistent);
    }
  }

  uint8_t lock_param_count() const {
    return DeviceParamResolver::slot(param_device_slot(), param_dest())
        .lock_param_count();
  }
  uint8_t lock_slot_count() const {
    return kind_ == KIND_MD ? NUM_LOCKS : STEPSEQ_NUM_LOCKS;
  }
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

  uint8_t length() const {
    return kind_ == KIND_MD ? tracks_.md->length : tracks_.stepseq->length;
  }
  uint8_t speed() const {
    return kind_ == KIND_MD ? tracks_.md->speed : tracks_.stepseq->speed;
  }
  uint8_t step_count() const {
    return kind_ == KIND_MD ? tracks_.md->step_count
                            : tracks_.stepseq->step_count;
  }
  uint8_t track_index() const {
    return kind_ == KIND_MD ? tracks_.md->track_number
                            : tracks_.stepseq->track_number;
  }
  uint8_t mute_state() const {
    return kind_ == KIND_MD ? tracks_.md->mute_state
                            : tracks_.stepseq->mute_state;
  }
  void set_mute_state(uint8_t state) {
    if (kind_ == KIND_MD) {
      tracks_.md->mute_state = state;
    } else {
      tracks_.stepseq->mute_state = state;
    }
  }
  void set_length(uint8_t len, bool expand = false) {
    if (kind_ == KIND_MD) {
      tracks_.md->set_length(len, expand);
    } else {
      tracks_.stepseq->set_length(len, expand);
    }
  }
  bool request_speed_change(uint8_t new_speed);

  uint8_t condition_count() const {
    return kind_ == KIND_MD ? NUM_TRIG_CONDITIONS
                            : STEPSEQ_NUM_TRIG_CONDITIONS - 1;
  }
  void condition_label(uint8_t condition, bool plock, bool marker,
                       char *out) const;
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

  uint8_t timing_encoder_min() const { return kind_ == KIND_MD ? 1 : 0; }
  uint8_t timing_encoder_center() const {
    return kind_ == KIND_MD ? SeqTrack::get_timing_mid(tracks_.md->speed)
                            : 127;
  }
  uint8_t timing_encoder_max() const {
    return kind_ == KIND_MD
               ? (uint8_t)(SeqTrack::get_timing_mid(tracks_.md->speed) * 2 - 1)
               : 254;
  }
  uint8_t timing_display_mid() const {
    return kind_ == KIND_MD ? SeqTrack::get_timing_mid(tracks_.md->speed) : 0;
  }
  uint8_t timing_encoder_for_step(uint8_t step) const {
    if (kind_ == KIND_MD) {
      uint8_t timing = tracks_.md->timing[step];
      return timing == 0 ? SeqTrack::get_timing_mid(tracks_.md->speed) : timing;
    }
    return (uint8_t)(tracks_.stepseq->microtiming[step] + 127);
  }
  int8_t microtiming_from_encoder(uint8_t encoder_value) const {
    return (int8_t)((int16_t)encoder_value - 127);
  }
  int8_t microtiming_for_step(uint8_t step) const {
    return kind_ == KIND_MD ? 0 : tracks_.stepseq->microtiming[step];
  }

  void rotate_left() {
    if (kind_ == KIND_MD) {
      tracks_.md->rotate_left();
    } else {
      tracks_.stepseq->rotate_left();
    }
  }
  void rotate_right() {
    if (kind_ == KIND_MD) {
      tracks_.md->rotate_right();
    } else {
      tracks_.stepseq->rotate_right();
    }
  }
  void reverse() {
    if (kind_ == KIND_MD) {
      tracks_.md->reverse();
    } else {
      tracks_.stepseq->reverse();
    }
  }
  void transpose(int8_t offset) {
    if (kind_ == KIND_MD) {
      tracks_.md->transpose(offset);
    } else {
      tracks_.stepseq->transpose(offset);
    }
  }
  void get_mask(uint64_t *mask, uint8_t ui_mask) const;
  bool get_step(uint8_t step, uint8_t ui_mask) const;
  void set_step(uint8_t step, uint8_t ui_mask, bool value);

  uint8_t conditional_id(uint8_t step) const {
    return kind_ == KIND_MD ? tracks_.md->steps[step].cond_id
                            : tracks_.stepseq->steps[step].cond_id;
  }
  bool conditional_plock(uint8_t step) const {
    return kind_ == KIND_MD ? tracks_.md->steps[step].cond_plock
                            : tracks_.stepseq->steps[step].cond_plock;
  }
  void set_conditional(uint8_t step, uint8_t condition, bool plock) {
    if (kind_ == KIND_MD) {
      tracks_.md->steps[step].cond_id = condition;
      tracks_.md->steps[step].cond_plock = plock;
    } else {
      tracks_.stepseq->steps[step].cond_id = condition;
      tracks_.stepseq->steps[step].cond_plock = plock;
    }
  }
  void clear_conditional(uint8_t step) { set_conditional(step, 0, false); }
  void set_timing_from_encoder(uint8_t step, uint8_t encoder_value) {
    if (kind_ == KIND_MD) {
      tracks_.md->timing[step] = encoder_value;
    } else {
      tracks_.stepseq->microtiming[step] =
          (int8_t)((int16_t)encoder_value - 127);
    }
  }
  void set_pattern_step_from_edit(uint8_t step, uint8_t condition_knob,
                                  uint8_t timing_encoder);
  void reset_timing(uint8_t step) {
    if (kind_ == KIND_MD) {
      tracks_.md->timing[step] = tracks_.md->get_timing_mid();
    } else {
      tracks_.stepseq->microtiming[step] = 0;
    }
  }
  void clear_mute(uint8_t step) {
    if (kind_ == KIND_MD) {
      tracks_.md->mute_mask &= ~(1ULL << step);
    } else {
      tracks_.stepseq->mute_mask &= ~(1ULL << step);
    }
  }
  void toggle_mute(uint8_t step) {
    if (kind_ == KIND_MD) {
      tracks_.md->mute_mask ^= (1ULL << step);
    } else {
      tracks_.stepseq->mute_mask ^= (1ULL << step);
    }
  }
  uint64_t mute_mask() const {
    return kind_ == KIND_MD ? tracks_.md->mute_mask
                            : tracks_.stepseq->mute_mask;
  }

  void enable_step_locks(uint8_t step) {
    if (kind_ == KIND_MD) {
      tracks_.md->enable_step_locks(step);
    } else {
      tracks_.stepseq->enable_step_locks(step);
    }
  }
  void clear_step_lock(uint8_t step, uint8_t param_id) {
    if (kind_ == KIND_MD) {
      tracks_.md->clear_step_lock(step, param_id);
    } else {
      tracks_.stepseq->clear_step_lock(step, param_id);
    }
  }
  void clear_step_locks(uint8_t step) {
    if (kind_ == KIND_MD) {
      tracks_.md->clear_step_locks(step);
    } else {
      tracks_.stepseq->clear_step_locks(step);
    }
  }
  void clear_param_locks(uint8_t param_id) {
    if (kind_ == KIND_MD) {
      tracks_.md->clear_param_locks(param_id);
    } else {
      tracks_.stepseq->clear_param_locks(param_id);
    }
  }
  void clear_locks() {
    if (kind_ == KIND_MD) {
      tracks_.md->clear_locks();
    } else {
      tracks_.stepseq->clear_locks();
    }
  }
  bool set_track_locks(uint8_t step, uint8_t param_id, uint8_t value) {
    return kind_ == KIND_MD
               ? tracks_.md->set_track_locks(step, param_id, value)
               : tracks_.stepseq->set_track_locks(step, param_id, value);
  }
  bool step_has_lock(uint8_t step, uint8_t lock_idx) const {
    return kind_ == KIND_MD ? tracks_.md->steps[step].is_lock(lock_idx)
                            : tracks_.stepseq->steps[step].is_lock(lock_idx);
  }
  int8_t find_param(uint8_t param_id) const {
    uint8_t lock_idx = kind_ == KIND_MD
                           ? tracks_.md->find_param(param_id)
                           : tracks_.stepseq->find_param(param_id);
    return lock_idx == 255 ? -1 : (int8_t)lock_idx;
  }
  uint8_t pitch_lock_param_id() const {
    return DeviceParamResolver::slot(param_device_slot(), param_dest())
        .pitch_lock_param();
  }
  uint8_t get_track_lock_implicit(uint8_t step, uint8_t param_id) {
    return kind_ == KIND_MD
               ? tracks_.md->get_track_lock_implicit(step, param_id)
               : tracks_.stepseq->get_track_lock_implicit(step, param_id);
  }
  void clear_track(bool locks = true) {
    if (kind_ == KIND_MD) {
      tracks_.md->clear_track(locks);
    } else {
      tracks_.stepseq->clear_track(locks);
    }
  }
  void clear_step(uint8_t step);
  void clean_params() {
    if (kind_ == KIND_MD) {
      tracks_.md->clean_params();
    } else {
      tracks_.stepseq->clean_params();
    }
  }
  void copy_step(uint8_t step, MDSeqStep *md_step, StepSeqStep *stepseq_step) {
    if (kind_ == KIND_MD) {
      tracks_.md->copy_step(step, md_step);
    } else {
      tracks_.stepseq->copy_step(step, stepseq_step);
    }
  }
  void paste_step(uint8_t step, MDSeqStep *md_step,
                  StepSeqStep *stepseq_step) {
    if (kind_ == KIND_MD) {
      tracks_.md->paste_step(step, md_step);
    } else {
      tracks_.stepseq->paste_step(step, stepseq_step);
    }
  }
  void set_track_pitch(uint8_t step, uint8_t pitch) {
    if (kind_ == KIND_MD) {
      tracks_.md->set_track_pitch(step, pitch);
    } else {
      tracks_.stepseq->set_track_pitch(step, pitch);
    }
  }
  void get_step_locks(uint8_t step, uint8_t *params,
                      bool ignore_locks_disabled) {
    if (kind_ == KIND_MD) {
      tracks_.md->get_step_locks(step, params, ignore_locks_disabled);
    } else {
      tracks_.stepseq->get_step_locks(step, params, ignore_locks_disabled);
    }
  }
  void record_track(uint8_t velocity) {
    if (kind_ == KIND_MD) {
      tracks_.md->record_track(velocity);
    } else {
      tracks_.stepseq->record_track(velocity);
    }
  }
  void record_track_locks(uint8_t param_id, uint8_t value) {
    if (kind_ == KIND_MD) {
      tracks_.md->record_track_locks(param_id, value);
    } else {
      tracks_.stepseq->record_track_locks(param_id, value);
    }
  }
  bool preview_step(uint8_t step);

private:
  uint8_t param_device_slot() const { return device_slot_; }
  uint8_t param_dest() const { return track_index() + 1; }
  DeviceContext param_context() const {
    uint8_t slot = param_device_slot();
    uint8_t device_idx = slot == 0 ? 0 : slot - 1;
    return device_manager.context_for_device(device_idx);
  }
  uint8_t param_target() const { return track_index(); }
  DeviceStepEditCapability *step_edit() const {
    return param_context().device()->step_edit();
  }

  enum Kind : uint8_t { KIND_MD, KIND_STEPSEQ };
  union {
    MDSeqTrack *md;
    StepSeqDataTrack *stepseq;
  } tracks_;
  Kind kind_;
  uint8_t device_slot_;
};
