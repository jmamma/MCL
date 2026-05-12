#pragma once

#include "../Drivers/MD/MD.h"
#include "SeqDefines.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

class SeqStepTrackMdBackend {
public:
  explicit SeqStepTrackMdBackend(MDSeqTrack &track, uint8_t device_slot = 1)
      : track_(&track) {
    (void)device_slot;
  }

  bool uses_signed_microtiming() const { return false; }
  bool clears_mute_on_pattern_clear() const { return false; }
  bool shows_lock_value_popup() const { return true; }
  bool uses_kit_sound() const { return true; }
  bool selects_track_locally() const { return false; }
  bool uses_step_pitch() const { return true; }

  bool configure_panel(char *info, size_t info_len, uint8_t &pitch_encoder_max,
                       bool &is_midi_model) const {
    return configure_md_kit_panel(info, (uint8_t)info_len, &pitch_encoder_max,
                                  &is_midi_model);
  }

  bool step_editor_available() const { return MD.global.extendedMode == 2; }

  void set_step_edit_rec_mode(uint8_t mode) const { MD.set_rec_mode(mode); }

  void sync_step_edit() const {
    MD.sync_seqtrack(length(), speed(), step_count());
  }

  void set_step_edit_trig_leds(uint16_t mask, uint8_t mode,
                               uint8_t blink = 0) const {
    MD.set_trigleds(mask, (TrigLEDMode)mode, blink);
  }

  void set_live_param_update(bool enabled) const {
    if (enabled) {
      MD.midi_events.enable_live_kit_update();
    } else {
      MD.midi_events.disable_live_kit_update();
    }
    seq_step_set_md_linked_param_update(enabled);
  }

  bool uses_note_pitch() const { return md_kit_uses_note_pitch(); }
  uint8_t note_from_pitch_lock(uint8_t pitch) const {
    return md_kit_note_from_pitch(pitch);
  }
  uint8_t pitch_lock_from_note(uint8_t note, uint8_t fine_tune = 255) const {
    return md_kit_pitch_from_note(note, fine_tune);
  }
  uint8_t default_pitch_lock() const { return md_kit_default_pitch(); }

  bool param_from_key(uint8_t key, uint8_t *param) const {
    return md_param_from_key(key, param);
  }

  bool key_for_param(uint8_t param, uint8_t *key) const {
    return md_key_for_param(param, key);
  }

  bool begin_param_editor(uint8_t *params, uint8_t count) const {
    if (params == nullptr || count < MD_PARAMS_PER_TRACK) {
      return false;
    }
    MD.activate_encoder_interface(params);
    return true;
  }

  void end_param_editor() const {
    if (MD.encoder_interface) {
      MD.deactivate_encoder_interface();
    }
  }

  void close_microtiming() const { MD.draw_close_microtiming(); }
  void clear_step_edit_popup() const { MD.popup_text(127, 2); }
  void popup_text(char *text, uint8_t persistent = 0) const {
    MD.popup_text(text, persistent);
  }

  uint8_t lock_param_count() const { return MD_PARAMS_PER_TRACK; }
  uint8_t lock_slot_count() const { return NUM_LOCKS; }

  bool lock_param_info(uint8_t param_id, SeqStepLockParamInfo &info) const {
    if (param_id >= MD_PARAMS_PER_TRACK) {
      return false;
    }
    info = SeqStepLockParamInfo();
    info.active = true;
    info.sendable = true;
    info.param_id = param_id;
    info.ctrl = param_id;
    info.default_value = current_lock_value(param_id);
    info.current_value = info.default_value;
    return true;
  }

  uint8_t current_lock_value(uint8_t param_id) const {
    return param_id < MD_PARAMS_PER_TRACK && track_index() < NUM_MD_TRACKS
               ? MD.kit.params[track_index()][param_id]
               : 0;
  }

  bool copy_lock_param_label(uint8_t param_id, char *dst,
                             size_t dst_len) const {
    if (dst == nullptr || dst_len == 0 || param_id >= MD_PARAMS_PER_TRACK ||
        track_index() >= NUM_MD_TRACKS) {
      return false;
    }
    const char *label = model_param_name(MD.kit.get_model(track_index()),
                                         param_id);
    return copy_short_label(label, dst, (uint8_t)dst_len, 3);
  }

  uint8_t length() const { return track_->length; }
  uint8_t speed() const { return track_->speed; }
  uint8_t step_count() const { return track_->step_count; }
  uint8_t track_index() const { return track_->track_number; }
  uint8_t mute_state() const { return track_->mute_state; }
  void set_mute_state(uint8_t state) { track_->mute_state = state; }

  void set_length(uint8_t len, bool expand = false) {
    track_->set_length(len, expand);
  }

  bool request_speed_change(uint8_t new_speed) {
    return track_->request_speed_change(new_speed);
  }

  uint8_t condition_count() const { return NUM_TRIG_CONDITIONS; }

  void condition_label(uint8_t condition, bool plock, bool marker,
                       char *out) const {
    if (out == nullptr) {
      return;
    }
    static const char PROGMEM ptab[] = "12579";

    char a = 'L';
    char b = '1';
    if (condition != 0) {
      if (condition <= 8) {
        b = '0' + condition;
      } else if (condition <= 13) {
        a = 'P';
        b = pgm_read_byte(&ptab[condition - 9]);
      } else if (condition == 14) {
        a = '1';
        b = 'S';
      }
    }

    out[0] = a;
    out[1] = b;
    uint8_t i = 2;
    if (plock) {
      out[i++] = marker ? '+' : '^';
    }
    out[i] = '\0';
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

  uint8_t timing_encoder_min() const { return 1; }
  uint8_t timing_encoder_center() const { return timing_mid(); }
  uint8_t timing_encoder_max() const { return timing_mid() * 2 - 1; }
  uint8_t timing_display_mid() const { return timing_mid(); }

  uint8_t timing_encoder_for_step(uint8_t step) const {
    uint8_t timing = track_->timing[step];
    return timing == 0 ? timing_mid() : timing;
  }

  int8_t microtiming_from_encoder(uint8_t encoder_value) const {
    return (int8_t)((int16_t)encoder_value - 127);
  }

  int8_t microtiming_for_step(uint8_t step) const {
    (void)step;
    return 0;
  }

  void rotate_left() { track_->rotate_left(); }
  void rotate_right() { track_->rotate_right(); }
  void reverse() { track_->reverse(); }
  void transpose(int8_t offset) { track_->transpose(offset); }
  void get_mask(uint64_t *mask, uint8_t ui_mask) const {
    track_->get_mask(mask, ui_mask);
  }
  bool get_step(uint8_t step, uint8_t ui_mask) const {
    return track_->get_step(step, ui_mask);
  }
  void set_step(uint8_t step, uint8_t ui_mask, bool value) {
    track_->set_step(step, ui_mask, value);
  }

  uint8_t conditional_id(uint8_t step) const {
    return track_->steps[step].cond_id;
  }
  bool conditional_plock(uint8_t step) const {
    return track_->steps[step].cond_plock;
  }
  void set_conditional(uint8_t step, uint8_t condition, bool plock) {
    track_->steps[step].cond_id = condition;
    track_->steps[step].cond_plock = plock;
  }
  void clear_conditional(uint8_t step) { set_conditional(step, 0, false); }

  void set_timing_from_encoder(uint8_t step, uint8_t encoder_value) {
    track_->timing[step] = encoder_value;
  }

  void set_pattern_step_from_edit(uint8_t step, uint8_t condition_knob,
                                  uint8_t timing_encoder) {
    bool cond_plock;
    uint8_t condition = step_conditional_from_knob(condition_knob, &cond_plock);
    track_->steps[step].trig = true;
    track_->steps[step].cond_id = condition;
    track_->steps[step].cond_plock = cond_plock;
    track_->timing[step] = timing_encoder;
  }

  void reset_timing(uint8_t step) { track_->timing[step] = timing_mid(); }
  void clear_mute(uint8_t step) { track_->mute_mask &= ~(1ULL << step); }
  void toggle_mute(uint8_t step) { track_->mute_mask ^= (1ULL << step); }
  uint64_t mute_mask() const { return track_->mute_mask; }

  void enable_step_locks(uint8_t step) { track_->enable_step_locks(step); }
  void clear_step_lock(uint8_t step, uint8_t param_id) {
    track_->clear_step_lock(step, param_id);
  }
  void clear_step_locks(uint8_t step) { track_->clear_step_locks(step); }
  void clear_param_locks(uint8_t param_id) {
    track_->clear_param_locks(param_id);
  }
  void clear_locks() { track_->clear_locks(); }
  bool set_track_locks(uint8_t step, uint8_t param_id, uint8_t value) {
    return track_->set_track_locks(step, param_id, value);
  }
  bool step_has_lock(uint8_t step, uint8_t lock_idx) const {
    return track_->steps[step].is_lock(lock_idx);
  }

  int8_t find_param(uint8_t param_id) const {
    uint8_t lock_idx = track_->find_param(param_id);
    return lock_idx == 255 ? -1 : (int8_t)lock_idx;
  }

  uint8_t pitch_lock_param_id() const { return 0; }
  uint8_t get_track_lock_implicit(uint8_t step, uint8_t param_id) {
    return track_->get_track_lock_implicit(step, param_id);
  }
  void clear_track(bool locks = true) { track_->clear_track(locks); }
  void clear_step(uint8_t step) {
    MDSeqStep empty_step;
    memset(&empty_step, 0, sizeof(empty_step));
    track_->paste_step(step, &empty_step);
  }
  void clean_params() { track_->clean_params(); }
  void copy_step(uint8_t step, MDSeqStep *md_step) {
    track_->copy_step(step, md_step);
  }
  void paste_step(uint8_t step, MDSeqStep *md_step) {
    track_->paste_step(step, md_step);
  }
  void set_track_pitch(uint8_t step, uint8_t pitch) {
    track_->set_track_pitch(step, pitch);
  }
  void get_step_locks(uint8_t step, uint8_t *params,
                      bool ignore_locks_disabled) {
    track_->get_step_locks(step, params, ignore_locks_disabled);
  }
  void record_track(uint8_t velocity) { track_->record_track(velocity); }
  void record_track_locks(uint8_t param_id, uint8_t value) {
    track_->record_track_locks(param_id, value);
  }
  bool preview_step(uint8_t step) {
    track_->send_parameter_locks(step, true);
    MD.triggerTrack(track_->track_number, 127);
    return true;
  }

private:
  uint8_t timing_mid() const { return SeqTrack::get_timing_mid(track_->speed); }

  bool copy_short_label(const char *label, char *out, uint8_t len,
                        uint8_t max_chars) const {
    if (label == nullptr || out == nullptr || len == 0) {
      return false;
    }
    uint8_t pos = 0;
    while (label[pos] != '\0' && pos + 1 < len && pos < max_chars) {
      out[pos] = label[pos];
      ++pos;
    }
    out[pos] = '\0';
    if (pos == 2 && pos + 1 < len) {
      out[pos++] = ' ';
      out[pos] = '\0';
    }
    return true;
  }

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
    int8_t pitch =
        (int8_t)pgm_read_byte(&tuning->tuning[note]) + (int8_t)fine_tune - 32;
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

  MDSeqTrack *track_;
};
