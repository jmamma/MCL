#pragma once

#include <inttypes.h>

class MidiDevice;
class MidiUartClass;
class PerfData;
class SeqTrack;
struct MidiDeviceMixerParam;
struct MidiDeviceParamInfo;

class DeviceMixerCapability {
public:
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

protected:
  MidiDevice &device_;
};

#if !defined(__AVR__)
class DeviceParamCapability {
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

protected:
  MidiDevice &device_;
};
#endif

class DevicePanelCapability {
public:
  virtual void set_key_repeat(uint8_t enabled);
};
