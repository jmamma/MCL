#pragma once

#include "DeviceContext.h"
#include <inttypes.h>

class MidiDevice;
class MidiClass;
class MidiUartClass;
class PerfData;
class SeqExtStepTrackApi;
class SeqStepTrackRef;
class SeqTrack;
struct MidiDeviceMixerParam;
struct MidiDeviceParamInfo;

#if defined(PLATFORM_TBD)
using MidiDeviceMixerValue = int16_t;
#else
using MidiDeviceMixerValue = uint8_t;
#endif

class DeviceCapability {
protected:
  explicit DeviceCapability(MidiDevice &device) : device_(device) {}

  MidiDevice &device_;
};

class DeviceMixerCapability : public DeviceCapability {
public:
  static constexpr uint8_t MUTE_PARAM = 255;

  explicit DeviceMixerCapability(MidiDevice &device,
                                 uint8_t default_param = 0,
                                 uint8_t mute_param = MUTE_PARAM);

#if defined(__AVR__)
  virtual uint8_t track_count(const DeviceContext &ctx) const = 0;
  virtual SeqTrack *seq_track(const DeviceContext &ctx, uint8_t track) = 0;
#else
  virtual uint8_t track_count(const DeviceContext &ctx) const;
  virtual SeqTrack *seq_track(const DeviceContext &ctx, uint8_t track);
#endif
  uint8_t default_param() const;
  virtual bool param(const DeviceContext &ctx, uint8_t track, uint8_t param_idx,
                     MidiDeviceMixerParam *out) = 0;
  virtual bool set_param(const DeviceContext &ctx, uint8_t track,
                         uint8_t param_idx, MidiDeviceMixerValue value,
                         bool send = true) = 0;
  virtual bool set_seq_mute_state(const DeviceContext &ctx, uint8_t track,
                                  bool mute);
  virtual void mute_track(const DeviceContext &ctx, uint8_t track, bool mute,
                          MidiUartClass *uart_ = nullptr);
#if !defined(__AVR__)
  virtual void fill_track(const DeviceContext &ctx, uint8_t track, bool fill,
                          MidiUartClass *uart_ = nullptr);
#endif
  virtual void set_record_mutes(const DeviceContext &ctx, uint8_t track,
                                bool state, bool clear = false) = 0;
  virtual uint8_t trig_group(const DeviceContext &ctx, uint8_t track) const;
  virtual void select_track(const DeviceContext &ctx, uint8_t track);
  virtual void restore_track_params(const DeviceContext &ctx, uint8_t track);
  virtual bool parse_cc(const DeviceContext &ctx, uint8_t channel, uint8_t cc,
                        uint8_t *track, uint8_t *param) const = 0;
  bool is_mute_param(uint8_t param) const;
  virtual void update_from_cc(const DeviceContext &ctx, uint8_t track,
                              uint8_t param, MidiDeviceMixerValue value);

private:
  uint8_t default_param_;
  uint8_t mute_param_;
};

class ExtMixerCapability : public DeviceMixerCapability {
public:
  ExtMixerCapability(MidiDevice &device, uint8_t *levels,
                     bool require_level_cc = false)
      : DeviceMixerCapability(device), levels_(levels),
        require_level_cc_(require_level_cc) {}

  virtual uint8_t track_count(const DeviceContext &ctx) const override;
  virtual SeqTrack *seq_track(const DeviceContext &ctx,
                              uint8_t track) override;
  virtual bool param(const DeviceContext &ctx, uint8_t track,
                     uint8_t param_idx,
                     MidiDeviceMixerParam *out) override;
  virtual bool set_param(const DeviceContext &ctx, uint8_t track,
                         uint8_t param_idx, MidiDeviceMixerValue value,
                         bool send = true) override;
  virtual bool set_seq_mute_state(const DeviceContext &ctx, uint8_t track,
                                  bool mute) override;
  virtual void set_record_mutes(const DeviceContext &ctx, uint8_t track,
                                bool state, bool clear = false) override;
  virtual bool parse_cc(const DeviceContext &ctx, uint8_t channel, uint8_t cc,
                        uint8_t *track, uint8_t *param) const override;
  virtual void update_from_cc(const DeviceContext &ctx, uint8_t track,
                              uint8_t param, MidiDeviceMixerValue value) override;

protected:
  virtual void send_level(uint8_t track, uint8_t level, bool send) = 0;

private:
  uint8_t *levels_;
  bool require_level_cc_;
};

#if !defined(__AVR__)
class DeviceStepTrackCapability : public DeviceCapability {
public:
  explicit DeviceStepTrackCapability(MidiDevice &device);

  virtual bool available(const DeviceContext &ctx) const;
  virtual uint8_t track_count(const DeviceContext &ctx) const;
  virtual SeqStepTrackRef track(const DeviceContext &ctx, uint8_t track) const;
  virtual SeqStepTrackRef active_track(const DeviceContext &ctx) const;
  virtual bool parses_kit_cc(const DeviceContext &ctx) const;
  virtual bool parse_kit_cc(const DeviceContext &ctx, uint8_t channel,
                            uint8_t cc, uint8_t *track, uint8_t *param) const;
};

class DeviceExtStepTrackCapability : public DeviceCapability {
public:
  explicit DeviceExtStepTrackCapability(MidiDevice &device);

  virtual bool available(const DeviceContext &ctx) const;
  virtual uint8_t track_count(const DeviceContext &ctx) const;
  virtual SeqExtStepTrackApi track(const DeviceContext &ctx, uint8_t i) const;
  virtual SeqExtStepTrackApi active_track(const DeviceContext &ctx) const;
  virtual bool track_for_channel(const DeviceContext &ctx, uint8_t channel,
                                  uint8_t *track_index) const;
  virtual MidiClass *input_midi(const DeviceContext &ctx) const;
  virtual bool is_mute_cc(const DeviceContext &ctx, uint8_t cc) const;
};

class DeviceParamCapability : public DeviceCapability {
public:
  explicit DeviceParamCapability(MidiDevice &device);

  virtual uint8_t target_count(const DeviceContext &ctx) const;
  virtual uint8_t param_count(const DeviceContext &ctx, uint8_t target) const;
  virtual bool target_label(const DeviceContext &ctx, uint8_t target,
                            char *out, uint8_t len) const;
  virtual bool param_label(const DeviceContext &ctx, uint8_t target,
                           uint8_t param, char *out, uint8_t len);
  virtual bool get_param(const DeviceContext &ctx, uint8_t target,
                         uint8_t param, uint8_t *value);
  virtual bool set_param(const DeviceContext &ctx, uint8_t target,
                         uint8_t param, uint8_t value,
                         MidiUartClass *uart_ = nullptr,
                         bool update_kit = false);
  virtual uint8_t sequencer_lock_param_count(const DeviceContext &ctx,
                                             uint8_t target) const;
  virtual bool sequencer_lock_param_info(const DeviceContext &ctx,
                                         uint8_t target, uint8_t param,
                                         MidiDeviceParamInfo *info);
  virtual bool sequencer_lock_param_label(const DeviceContext &ctx,
                                          uint8_t target, uint8_t param,
                                          char *out, uint8_t len);
  virtual bool sequencer_uses_step_pitch(const DeviceContext &ctx,
                                         uint8_t target) const;
  virtual uint8_t sequencer_pitch_lock_param(const DeviceContext &ctx,
                                             uint8_t target) const;
};

class DevicePerfCapability : public DeviceCapability {
public:
  explicit DevicePerfCapability(MidiDevice &device);

  virtual bool perf_param_from_key(const DeviceContext &ctx, uint8_t target,
                                   uint8_t key, uint8_t *param);
  virtual bool perf_key_for_param(const DeviceContext &ctx, uint8_t target,
                                  uint8_t param, uint8_t *key);
  virtual bool perf_begin_param_editor(const DeviceContext &ctx,
                                       uint8_t target, uint8_t *params,
                                       uint8_t count);
  virtual void perf_end_param_editor(const DeviceContext &ctx);
  virtual void perf_set_rec_mode(const DeviceContext &ctx, uint8_t mode);
  virtual bool perf_scene_autofill(const DeviceContext &ctx,
                                   uint8_t dest_offset, PerfData *data,
                                   uint8_t scene);
};
#endif

#if !defined(__AVR__)
class DeviceStepEditCapability : public DeviceCapability {
public:
  explicit DeviceStepEditCapability(MidiDevice &device);

  virtual bool available(const DeviceContext &ctx) const;
  virtual void set_rec_mode(const DeviceContext &ctx, uint8_t mode);
  virtual void sync_track(const DeviceContext &ctx, uint8_t length,
                          uint8_t speed, uint8_t step_count,
                          uint8_t swing_amount = 0x7F);
  virtual void set_trig_leds(const DeviceContext &ctx, uint16_t mask,
                             uint8_t mode, uint8_t blink = 0);
  virtual void set_live_param_update(const DeviceContext &ctx, bool enabled);
  virtual bool configure_kit_sound_panel(const DeviceContext &ctx,
                                         uint8_t target, char *info,
                                         uint8_t info_len, uint8_t *pitch_max,
                                         bool *is_midi_model) const;
  virtual bool kit_sound_uses_note_pitch(const DeviceContext &ctx,
                                         uint8_t target) const;
  virtual bool kit_sound_voice_allocatable(const DeviceContext &ctx,
                                           uint8_t target) const;
  virtual uint8_t kit_sound_default_pitch(const DeviceContext &ctx,
                                          uint8_t target) const;
  virtual uint8_t kit_sound_note_from_pitch(const DeviceContext &ctx,
                                            uint8_t target,
                                            uint8_t pitch) const;
  virtual uint8_t kit_sound_pitch_from_note(const DeviceContext &ctx,
                                            uint8_t target, uint8_t note,
                                            uint8_t fine_tune) const;
  virtual bool param_from_key(const DeviceContext &ctx, uint8_t target,
                              uint8_t key, uint8_t *param) const;
  virtual bool key_for_param(const DeviceContext &ctx, uint8_t target,
                             uint8_t param, uint8_t *key) const;
  virtual bool begin_param_editor(const DeviceContext &ctx, uint8_t target,
                                  uint8_t *params, uint8_t count);
  virtual void end_param_editor(const DeviceContext &ctx);
  virtual void draw_microtiming(const DeviceContext &ctx, uint8_t speed,
                                uint8_t timing);
  virtual void draw_microtiming_signed(const DeviceContext &ctx, uint8_t speed,
                                       int8_t microtiming);
  virtual void close_microtiming(const DeviceContext &ctx);
  virtual void clear_popup(const DeviceContext &ctx);
  virtual void popup_text(const DeviceContext &ctx, char *text,
                          uint8_t persistent = 0);
  virtual bool parse_cc(const DeviceContext &ctx, uint8_t channel, uint8_t cc,
                        uint8_t *target, uint8_t *param) const;
};
#endif

#if !defined(__AVR__)
class DevicePanelCapability {
public:
  virtual void set_key_repeat(uint8_t enabled);
  virtual void set_rec_mode(uint8_t mode);
  virtual void sync_seqtrack(uint8_t length, uint8_t speed,
                             uint8_t step_count,
                             uint8_t swing_amount = 0x7F);
  virtual void popup_text(uint8_t action_string, uint8_t persistent = 0);
  virtual void popup_text(char *text, uint8_t persistent = 0);
  virtual void popup_text_P(const char *text_P, uint8_t persistent = 0);
  virtual void popup_text_P(const char *text1_P, const char *text2_P,
                            uint8_t persistent = 0);
};
#endif
