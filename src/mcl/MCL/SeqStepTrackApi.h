/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQSTEPTRACKAPI_H__
#define SEQSTEPTRACKAPI_H__

#include "../Drivers/MidiDevice.h"
#include "DeviceParamResolver.h"
#include "MCLSeq.h"
#if defined(__AVR__)
#include "MCLEncoder.h"
#endif
#if defined(PLATFORM_TBD)
#include "MCLSysConfig.h"
#include "MidiSetup.h"
#endif
#include "SeqDefines.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

extern uint8_t last_md_track;
#if defined(__AVR__)
extern MCLEncoder ptc_param_fine_tune;
#endif

using SeqStepLockParamInfo = MidiDeviceParamInfo;

class SeqStepTrackApi {
public:
  explicit SeqStepTrackApi(MDSeqTrack &track) : legacy_md_(&track)
#if !defined(__AVR__)
        ,
        step_(nullptr)
#endif
  {}

#if !defined(__AVR__)
  explicit SeqStepTrackApi(StepSeqDataTrack &track)
      : legacy_md_(nullptr), step_(&track) {}
#endif

  bool is_stepseq() const {
#if !defined(__AVR__)
    return step_ != nullptr;
#else
    return false;
#endif
  }

  bool uses_kit_sound() const {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return !step_->owns_sound_data();
    }
#endif
    return true;
  }

  bool selects_track_locally() const {
#if !defined(__AVR__)
    return is_stepseq() && step_->owns_sound_data();
#else
    return false;
#endif
  }

  bool uses_step_pitch() const {
    return uses_kit_sound() ||
           DeviceParamResolver::slot(param_device_slot(), param_dest())
               .uses_step_pitch();
  }

  bool configure_panel(char *info, size_t info_len,
                       uint8_t &pitch_encoder_max,
                       bool &is_midi_model) const {
    if (uses_kit_sound()) {
#if defined(__AVR__)
      return configure_md_kit_panel(info, (uint8_t)info_len,
                                    &pitch_encoder_max, &is_midi_model);
#else
      return step_edit()->configure_kit_sound_panel(
          param_device_idx(), param_target(), info, (uint8_t)info_len,
          &pitch_encoder_max, &is_midi_model);
#endif
    }
    pitch_encoder_max = uses_step_pitch() ? 127 : 1;
    is_midi_model = false;
    return DeviceParamResolver::slot(param_device_slot(), param_dest())
        .target_label(info, (uint8_t)info_len);
  }

  bool step_editor_available() const {
#if defined(__AVR__)
    return !uses_kit_sound() || MD.global.extendedMode == 2;
#else
    return !uses_kit_sound() || step_edit()->available(param_device_idx());
#endif
  }

  void set_step_edit_rec_mode(uint8_t mode) const {
    if (uses_kit_sound()) {
#if defined(__AVR__)
      MD.set_rec_mode(mode);
#else
      step_edit()->set_rec_mode(param_device_idx(), mode);
#endif
    }
  }

  void sync_step_edit() const {
    if (uses_kit_sound()) {
#if defined(__AVR__)
      MD.sync_seqtrack(length(), speed(), step_count());
#else
      step_edit()->sync_track(param_device_idx(), length(), speed(),
                              step_count());
#endif
    }
  }

  void set_step_edit_trig_leds(uint16_t mask, uint8_t mode,
                               uint8_t blink = 0) const {
    if (uses_kit_sound()) {
#if defined(__AVR__)
      MD.set_trigleds(mask, (TrigLEDMode)mode, blink);
#else
      step_edit()->set_trig_leds(param_device_idx(), mask, mode, blink);
#endif
    }
  }

  void set_live_param_update(bool enabled) const {
    if (uses_kit_sound()) {
#if defined(__AVR__)
      if (enabled) {
        MD.midi_events.enable_live_kit_update();
      } else {
        MD.midi_events.disable_live_kit_update();
      }
#else
      step_edit()->set_live_param_update(param_device_idx(), enabled);
#endif
    }
  }

  bool uses_note_pitch() const {
#if defined(__AVR__)
    return uses_kit_sound() ? md_kit_uses_note_pitch() : uses_step_pitch();
#else
    return uses_kit_sound()
               ? step_edit()->kit_sound_uses_note_pitch(param_device_idx(),
                                                        param_target())
               : uses_step_pitch();
#endif
  }

  uint8_t note_from_pitch_lock(uint8_t pitch) const {
#if defined(__AVR__)
    return uses_kit_sound() ? md_kit_note_from_pitch(pitch) : pitch;
#else
    return uses_kit_sound()
               ? step_edit()->kit_sound_note_from_pitch(param_device_idx(),
                                                        param_target(), pitch)
               : pitch;
#endif
  }

  uint8_t pitch_lock_from_note(uint8_t note, uint8_t fine_tune = 255) const {
#if defined(__AVR__)
    return uses_kit_sound() ? md_kit_pitch_from_note(note, fine_tune) : note;
#else
    return uses_kit_sound()
               ? step_edit()->kit_sound_pitch_from_note(
                     param_device_idx(), param_target(), note, fine_tune)
               : note;
#endif
  }

  uint8_t default_pitch_lock() const {
#if defined(__AVR__)
    return uses_kit_sound() ? md_kit_default_pitch() : 0;
#else
    return uses_kit_sound()
               ? step_edit()->kit_sound_default_pitch(param_device_idx(),
                                                      param_target())
               : 0;
#endif
  }

  bool param_from_key(uint8_t key, uint8_t *param) const {
#if defined(__AVR__)
    return uses_kit_sound() && md_param_from_key(key, param);
#else
    return uses_kit_sound() &&
           step_edit()->param_from_key(param_device_idx(), param_target(), key,
                                       param);
#endif
  }

  bool key_for_param(uint8_t param, uint8_t *key) const {
#if defined(__AVR__)
    return uses_kit_sound() && md_key_for_param(param, key);
#else
    return uses_kit_sound() &&
           step_edit()->key_for_param(param_device_idx(), param_target(), param,
                                      key);
#endif
  }

  bool begin_param_editor(uint8_t *params, uint8_t count) const {
#if defined(__AVR__)
    if (!uses_kit_sound() || params == nullptr || count < MD_PARAMS_PER_TRACK) {
      return false;
    }
    MD.activate_encoder_interface(params);
    return true;
#else
    return uses_kit_sound() &&
           step_edit()->begin_param_editor(param_device_idx(), param_target(),
                                           params, count);
#endif
  }

  void end_param_editor() const {
    if (uses_kit_sound()) {
#if defined(__AVR__)
      if (MD.encoder_interface) {
        MD.deactivate_encoder_interface();
      }
#else
      step_edit()->end_param_editor(param_device_idx());
#endif
    }
  }

  void close_microtiming() const {
    if (uses_kit_sound()) {
#if defined(__AVR__)
      MD.draw_close_microtiming();
#else
      step_edit()->close_microtiming(param_device_idx());
#endif
    }
  }

  void clear_step_edit_popup() const {
    if (uses_kit_sound()) {
#if defined(__AVR__)
      MD.popup_text(127, 2);
#else
      step_edit()->clear_popup(param_device_idx());
#endif
    }
  }

  void popup_text(char *text, uint8_t persistent = 0) const {
    if (uses_kit_sound()) {
#if defined(__AVR__)
      MD.popup_text(text, persistent);
#else
      step_edit()->popup_text(param_device_idx(), text, persistent);
#endif
    }
  }

  uint8_t lock_param_count() const {
    return DeviceParamResolver::slot(param_device_slot(), param_dest())
        .lock_param_count();
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
#if !defined(__AVR__)
    if (is_stepseq()) {
      return step_->length;
    }
#endif
    return legacy_md_->length;
  }

  uint8_t speed() const {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return step_->speed;
    }
#endif
    return legacy_md_->speed;
  }

  uint8_t step_count() const {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return step_->step_count;
    }
#endif
    return legacy_md_->step_count;
  }

  uint8_t track_index() const {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return step_->track_number;
    }
#endif
    return legacy_md_->track_number;
  }

  uint8_t mute_state() const {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return step_->mute_state;
    }
#endif
    return legacy_md_->mute_state;
  }

  void set_mute_state(uint8_t state) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->mute_state = state;
      return;
    }
#endif
    legacy_md_->mute_state = state;
  }

  void set_length(uint8_t len, bool expand = false) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->set_length(len, expand);
      return;
    }
#endif
    legacy_md_->set_length(len, expand);
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
    return legacy_md_->request_speed_change(new_speed);
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
    return legacy_md_->get_timing_mid();
  }

  uint8_t timing_encoder_max() const {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return 254;
    }
#endif
    return legacy_md_->get_timing_mid() * 2 - 1;
  }

  uint8_t timing_display_mid() const {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return 0;
    }
#endif
    return legacy_md_->get_timing_mid();
  }

  uint8_t timing_encoder_for_step(uint8_t step) const {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return (uint8_t)(microtiming_for_step(step) + 127);
    }
#endif
    uint8_t timing = legacy_md_->timing[step];
    return timing == 0 ? legacy_md_->get_timing_mid() : timing;
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
    legacy_md_->rotate_left();
  }

  void rotate_right() {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->rotate_right();
      return;
    }
#endif
    legacy_md_->rotate_right();
  }

  void reverse() {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->reverse();
      return;
    }
#endif
    legacy_md_->reverse();
  }

  void transpose(int8_t offset) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->transpose(offset);
      return;
    }
#endif
    legacy_md_->transpose(offset);
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
      legacy_md_->get_mask(mask, ui_mask);
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
    return legacy_md_->get_step(step, ui_mask);
  }

  void set_step(uint8_t step, uint8_t ui_mask, bool value) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->set_step(step, mask_for(ui_mask), value);
    } else {
#endif
      legacy_md_->set_step(step, ui_mask, value);
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
    return legacy_md_->steps[step].cond_id;
  }

  bool conditional_plock(uint8_t step) const {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return step_->steps[step].cond_plock;
    }
#endif
    return legacy_md_->steps[step].cond_plock;
  }

  void set_conditional(uint8_t step, uint8_t condition, bool plock) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->steps[step].cond_id = condition;
      step_->steps[step].cond_plock = plock;
    } else {
#endif
      legacy_md_->steps[step].cond_id = condition;
      legacy_md_->steps[step].cond_plock = plock;
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
      legacy_md_->timing[step] = encoder_value;
#if !defined(__AVR__)
    }
#endif
  }

  void set_pattern_step_from_edit(uint8_t step, uint8_t condition_knob,
                                  uint8_t timing_encoder) {
    bool cond_plock;
    uint8_t condition = step_conditional_from_knob(condition_knob, &cond_plock);
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->set_step(step, STEPSEQ_MASK_PATTERN, true);
      step_->steps[step].cond_id = condition;
      step_->steps[step].cond_plock = cond_plock;
      step_->microtiming[step] = microtiming_from_encoder(timing_encoder);
      return;
    }
#endif
    legacy_md_->steps[step].trig = true;
    legacy_md_->steps[step].cond_id = condition;
    legacy_md_->steps[step].cond_plock = cond_plock;
    legacy_md_->timing[step] = timing_encoder;
  }

  void reset_timing(uint8_t step) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->microtiming[step] = 0;
    } else {
#endif
      legacy_md_->timing[step] = legacy_md_->get_timing_mid();
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
      legacy_md_->mute_mask &= ~(1ULL << step);
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
      legacy_md_->mute_mask ^= (1ULL << step);
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
    return legacy_md_->mute_mask;
  }

  void enable_step_locks(uint8_t step) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->enable_step_locks(step);
    } else {
#endif
      legacy_md_->enable_step_locks(step);
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
      legacy_md_->clear_step_lock(step, param_id);
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
    legacy_md_->clear_step_locks(step);
  }

  void clear_param_locks(uint8_t param_id) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->clear_param_locks(param_id);
      return;
    }
#endif
    legacy_md_->clear_param_locks(param_id);
  }

  void clear_locks() {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->clear_locks();
      return;
    }
#endif
    legacy_md_->clear_locks();
  }

  bool set_track_locks(uint8_t step, uint8_t param_id, uint8_t value) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return step_->set_track_locks(step, param_id, value);
    }
#endif
    return legacy_md_->set_track_locks(step, param_id, value);
  }

  bool step_has_lock(uint8_t step, uint8_t lock_idx) const {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return step_->steps[step].is_lock(lock_idx);
    }
#endif
    return legacy_md_->steps[step].is_lock(lock_idx);
  }

  int8_t find_param(uint8_t param_id) const {
#if !defined(__AVR__)
    uint8_t lock_idx = is_stepseq() ? step_->find_param(param_id)
                                    : legacy_md_->find_param(param_id);
#else
    uint8_t lock_idx = legacy_md_->find_param(param_id);
#endif
    return lock_idx == 255 ? -1 : (int8_t)lock_idx;
  }

  uint8_t pitch_lock_param_id() const {
    return DeviceParamResolver::slot(param_device_slot(), param_dest())
        .pitch_lock_param();
  }

  uint8_t get_track_lock_implicit(uint8_t step, uint8_t param_id) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return step_->get_track_lock_implicit(step, param_id);
    }
#endif
    return legacy_md_->get_track_lock_implicit(step, param_id);
  }

  void clear_track(bool locks = true) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->clear_track(locks);
      return;
    }
#endif
    legacy_md_->clear_track(locks);
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
    legacy_md_->paste_step(step, &empty_step);
  }

  void clean_params() {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->clean_params();
      return;
    }
#endif
    legacy_md_->clean_params();
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
    legacy_md_->copy_step(step, md_step);
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
    legacy_md_->paste_step(step, md_step);
  }

  void set_track_pitch(uint8_t step, uint8_t pitch) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      step_->set_track_pitch(step, pitch);
    } else {
#endif
      legacy_md_->set_track_pitch(step, pitch);
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
      legacy_md_->get_step_locks(step, params, ignore_locks_disabled);
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
      legacy_md_->record_track(velocity);
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
      legacy_md_->record_track_locks(param_id, value);
#if !defined(__AVR__)
    }
#endif
  }

  bool preview_step(uint8_t step) {
#if !defined(__AVR__)
    if (is_stepseq()) {
      return step_->preview_step(step);
    }
#endif
    legacy_md_->send_parameter_locks(step, true);
    MD.triggerTrack(legacy_md_->track_number, 127);
    return true;
  }

private:
  uint8_t param_device_slot() const { return 1; }
  uint8_t param_dest() const { return track_index() + 1; }
  uint8_t param_device_idx() const {
    return DeviceParamResolver::slot_device_idx(param_device_slot());
  }
  uint8_t param_target() const { return track_index(); }
#if defined(__AVR__)
  bool configure_md_kit_panel(char *info, uint8_t info_len, uint8_t *pitch_max,
                              bool *is_midi_model) const {
    if (info == nullptr || info_len < 6 || track_index() >= NUM_MD_TRACKS) {
      return false;
    }
    uint8_t model = MD.kit.get_model(track_index());
    bool midi_model = ((model & 0xF0) == MID_01_MODEL);
    tuning_t const *tuning = MD.getKitModelTuning(track_index());
    if (is_midi_model != nullptr) {
      *is_midi_model = midi_model;
    }
    if (pitch_max != nullptr) {
      if (tuning) {
        *pitch_max = tuning->len - 1 + tuning->base;
      } else if (midi_model) {
        *pitch_max = 127;
      } else {
        *pitch_max = 1;
      }
    }
    const char *str1 = getMDMachineNameShort(model, 1);
    const char *str2 = getMDMachineNameShort(model, 2);
    copyMachineNameShort(str1, info);
    info[2] = '>';
    copyMachineNameShort(str2, info + 3);
    info[5] = '\0';
    return true;
  }

  bool md_kit_uses_note_pitch() const {
    if (track_index() >= NUM_MD_TRACKS) {
      return false;
    }
    uint8_t model = MD.kit.get_model(track_index());
    return ((model & 0xF0) == MID_01_MODEL) ||
           MD.getKitModelTuning(track_index()) != nullptr;
  }

  uint8_t md_kit_default_pitch() const {
    return track_index() < NUM_MD_TRACKS ? MD.kit.params[track_index()][0] : 0;
  }

  uint8_t md_kit_note_from_pitch(uint8_t pitch) const {
    if (track_index() >= NUM_MD_TRACKS) {
      return 255;
    }
    if ((MD.kit.models[track_index()] & 0xF0) == MID_01_MODEL) {
      return pitch;
    }
    tuning_t const *tuning = MD.getKitModelTuning(track_index());
    if (tuning == nullptr) {
      return 255;
    }
    pitch -= ptc_param_fine_tune.getValue() - 32;
    for (uint8_t i = 0; i < tuning->len; i++) {
      uint8_t cc = pgm_read_byte(&tuning->tuning[i]);
      if (cc >= pitch) {
        uint8_t note_offset = tuning->base - ((tuning->base / 12) * 12);
        return i + note_offset;
      }
    }
    return 255;
  }

  uint8_t md_kit_pitch_from_note(uint8_t note, uint8_t fine_tune) const {
    if (track_index() >= NUM_MD_TRACKS) {
      return 255;
    }
    if ((MD.kit.models[track_index()] & 0xF0) == MID_01_MODEL) {
      return note;
    }
    if (fine_tune == 255) {
      fine_tune = ptc_param_fine_tune.getValue();
    }
    tuning_t const *tuning = MD.getKitModelTuning(track_index());
    if (tuning == nullptr) {
      return 255;
    }
    uint8_t note_offset = tuning->base - ((tuning->base / 12) * 12);
    note -= note_offset;
    if (note >= tuning->len) {
      return 255;
    }
    int8_t pitch = (int8_t)pgm_read_byte(&tuning->tuning[note]) +
                   (int8_t)fine_tune - 32;
    if (pitch < 0) {
      return 0;
    }
    return pitch > 127 ? 127 : (uint8_t)pitch;
  }

  bool md_param_from_key(uint8_t key, uint8_t *param) const {
    if (param == nullptr || track_index() >= NUM_MD_TRACKS || key < 0x10 ||
        key > 0x17) {
      return false;
    }
    uint8_t value = MD.currentSynthPage * 8 + key - 0x10;
    if (value >= MD_PARAMS_PER_TRACK) {
      return false;
    }
    *param = value;
    return true;
  }

  bool md_key_for_param(uint8_t param, uint8_t *key) const {
    if (key == nullptr || track_index() >= NUM_MD_TRACKS ||
        param >= MD_PARAMS_PER_TRACK) {
      return false;
    }
    int16_t value = (int16_t)param - (int16_t)MD.currentSynthPage * 8 + 0x10;
    if (value < 0x10 || value > 0x17) {
      return false;
    }
    *key = (uint8_t)value;
    return true;
  }
#else
  DeviceStepEditCapability *step_edit() const {
    return DeviceParamResolver::slot_device(param_device_slot())->step_edit();
  }
#endif

  MDSeqTrack *legacy_md_;
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

inline bool seq_step_api_parse_kit_cc(uint8_t channel, uint8_t cc,
                                      uint8_t *track, uint8_t *param) {
#if defined(__AVR__)
  if (track == nullptr || param == nullptr) {
    return false;
  }
  MD.parseCC(channel, cc, track, param);
  return *track != 255;
#else
  MidiDevice *device = DeviceParamResolver::slot_device(1);
  return device != nullptr && device->step_edit()->parse_cc(
                                  DeviceParamResolver::slot_device_idx(1),
                                  channel, cc, track, param);
#endif
}

#endif /* SEQSTEPTRACKAPI_H__ */
