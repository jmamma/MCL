#pragma once

#include <inttypes.h>

class MidiDevice;
class MidiUartClass;
class PerfData;
class SeqStepTrackRef;
class SeqTrack;
struct MidiDeviceMixerParam;
struct MidiDeviceParamInfo;

class DeviceCapability {
protected:
  explicit DeviceCapability(MidiDevice &device) : device_(device) {}

  MidiDevice &device_;
};

class DeviceMixerCapability : public DeviceCapability {
public:
  static constexpr uint8_t MUTE_PARAM = 255;

  explicit DeviceMixerCapability(MidiDevice &device);

  virtual uint8_t track_count(uint8_t device_idx) const;
  virtual SeqTrack *seq_track(uint8_t device_idx, uint8_t track);
  virtual uint8_t default_param(uint8_t device_idx) const;
  virtual bool param(uint8_t device_idx, uint8_t track, uint8_t param_idx,
                     MidiDeviceMixerParam *out);
  virtual bool set_param(uint8_t device_idx, uint8_t track,
                         uint8_t param_idx, int16_t value,
                         bool send = true);
  virtual void mute_track(uint8_t device_idx, uint8_t track, bool mute,
                          MidiUartClass *uart_ = nullptr);
  virtual void set_record_mutes(uint8_t device_idx, uint8_t track, bool state,
                                bool clear = false);
  virtual uint8_t trig_group(uint8_t device_idx, uint8_t track) const;
  virtual void select_track(uint8_t device_idx, uint8_t track);
  virtual void restore_track_params(uint8_t device_idx, uint8_t track);
  virtual bool parse_cc(uint8_t device_idx, uint8_t channel, uint8_t cc,
                        uint8_t *track, uint8_t *param) const;
  virtual bool is_mute_param(uint8_t param) const;
  virtual void update_from_cc(uint8_t device_idx, uint8_t track,
                              uint8_t param, int16_t value);

};

class DeviceStepTrackCapability : public DeviceCapability {
public:
  explicit DeviceStepTrackCapability(MidiDevice &device);

  virtual bool available(uint8_t device_idx) const;
  virtual uint8_t track_count(uint8_t device_idx) const;
  virtual SeqStepTrackRef track(uint8_t device_idx, uint8_t track) const;
  virtual SeqStepTrackRef active_track(uint8_t device_idx) const;
  virtual bool parses_kit_cc(uint8_t device_idx) const;
  virtual bool parse_kit_cc(uint8_t device_idx, uint8_t channel, uint8_t cc,
                            uint8_t *track, uint8_t *param) const;
};

#if !defined(__AVR__)
class DeviceParamCapability : public DeviceCapability {
public:
  explicit DeviceParamCapability(MidiDevice &device);

  virtual uint8_t target_count(uint8_t device_idx) const;
  virtual uint8_t param_count(uint8_t device_idx, uint8_t target) const;
  virtual bool target_label(uint8_t device_idx, uint8_t target, char *out,
                            uint8_t len) const;
  virtual bool param_label(uint8_t device_idx, uint8_t target, uint8_t param,
                           char *out, uint8_t len);
  virtual bool get_param(uint8_t device_idx, uint8_t target, uint8_t param,
                         uint8_t *value);
  virtual bool set_param(uint8_t device_idx, uint8_t target, uint8_t param,
                         uint8_t value, MidiUartClass *uart_ = nullptr);
  virtual uint8_t sequencer_lock_param_count(uint8_t device_idx,
                                             uint8_t target) const;
  virtual bool sequencer_lock_param_info(uint8_t device_idx, uint8_t target,
                                         uint8_t param,
                                         MidiDeviceParamInfo *info);
  virtual bool sequencer_lock_param_label(uint8_t device_idx, uint8_t target,
                                          uint8_t param, char *out,
                                          uint8_t len);
  virtual bool sequencer_uses_step_pitch(uint8_t device_idx,
                                         uint8_t target) const;
  virtual uint8_t sequencer_pitch_lock_param(uint8_t device_idx,
                                             uint8_t target) const;

};

class DevicePerfCapability : public DeviceCapability {
public:
  explicit DevicePerfCapability(MidiDevice &device);

  virtual bool perf_param_from_key(uint8_t device_idx, uint8_t target,
                                   uint8_t key, uint8_t *param);
  virtual bool perf_key_for_param(uint8_t device_idx, uint8_t target,
                                  uint8_t param, uint8_t *key);
  virtual bool perf_begin_param_editor(uint8_t device_idx, uint8_t target,
                                       uint8_t *params, uint8_t count);
  virtual void perf_end_param_editor(uint8_t device_idx);
  virtual void perf_set_rec_mode(uint8_t device_idx, uint8_t mode);
  virtual bool perf_scene_autofill(uint8_t device_idx, uint8_t dest_offset,
                                   PerfData *data, uint8_t scene);

};
#endif

#if !defined(__AVR__)
class DeviceStepEditCapability : public DeviceCapability {
public:
  explicit DeviceStepEditCapability(MidiDevice &device);

  virtual bool available(uint8_t device_idx) const;
  virtual void set_rec_mode(uint8_t device_idx, uint8_t mode);
  virtual void sync_track(uint8_t device_idx, uint8_t length, uint8_t speed,
                          uint8_t step_count);
  virtual void set_trig_leds(uint8_t device_idx, uint16_t mask, uint8_t mode,
                             uint8_t blink = 0);
  virtual void set_live_param_update(uint8_t device_idx, bool enabled);
  virtual bool configure_kit_sound_panel(uint8_t device_idx, uint8_t target,
                                         char *info, uint8_t info_len,
                                         uint8_t *pitch_max,
                                         bool *is_midi_model) const;
  virtual bool kit_sound_uses_note_pitch(uint8_t device_idx,
                                         uint8_t target) const;
  virtual uint8_t kit_sound_default_pitch(uint8_t device_idx,
                                          uint8_t target) const;
  virtual uint8_t kit_sound_note_from_pitch(uint8_t device_idx, uint8_t target,
                                            uint8_t pitch) const;
  virtual uint8_t kit_sound_pitch_from_note(uint8_t device_idx, uint8_t target,
                                            uint8_t note,
                                            uint8_t fine_tune) const;
  virtual bool param_from_key(uint8_t device_idx, uint8_t target, uint8_t key,
                              uint8_t *param) const;
  virtual bool key_for_param(uint8_t device_idx, uint8_t target, uint8_t param,
                             uint8_t *key) const;
  virtual bool begin_param_editor(uint8_t device_idx, uint8_t target,
                                  uint8_t *params, uint8_t count);
  virtual void end_param_editor(uint8_t device_idx);
  virtual void close_microtiming(uint8_t device_idx);
  virtual void clear_popup(uint8_t device_idx);
  virtual void popup_text(uint8_t device_idx, char *text,
                          uint8_t persistent = 0);
  virtual bool parse_cc(uint8_t device_idx, uint8_t channel, uint8_t cc,
                        uint8_t *target, uint8_t *param) const;
};
#endif

class DevicePanelCapability {
public:
  virtual void set_key_repeat(uint8_t enabled);
  virtual void set_rec_mode(uint8_t mode);
#if !defined(__AVR__)
  virtual void sync_seqtrack(uint8_t length, uint8_t speed,
                             uint8_t step_count);
#endif
};
