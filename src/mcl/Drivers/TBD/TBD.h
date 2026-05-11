#pragma once

#include "platform.h"

#ifdef PLATFORM_TBD

#include "../MidiDevice.h"
#include "TbdP4SoundData.h"
#include <stdint.h>

class TbdDevice : public MidiDevice {
public:
  TbdDevice();

  virtual bool probe() override;
  virtual void disconnect(uint8_t device_idx) override;
  virtual void on_connection(uint8_t device_idx) override;
  virtual void init_grid_devices(uint8_t device_idx) override;
  void sync_grid_devices();
  virtual bool supports_capability(MidiDeviceCapability capability) const override;
  virtual void muteTrack(uint8_t track, bool mute = true,
                         MidiUartClass *uart_ = nullptr) override;
  virtual void triggerTrack(uint8_t track, uint8_t velocity,
                            MidiUartClass *uart_ = nullptr) override;
  virtual uint8_t mixer_default_param(uint8_t device_idx) const override;
  virtual bool mixer_param(uint8_t device_idx, uint8_t track,
                           uint8_t param_idx,
                           MidiDeviceMixerParam *param) override;
  virtual bool set_mixer_param(uint8_t device_idx, uint8_t track,
                               uint8_t param_idx, int16_t value,
                               bool send = true) override;
  virtual uint8_t param_target_count(uint8_t device_idx) const override;
  virtual uint8_t param_count(uint8_t device_idx, uint8_t target) const override;
  virtual bool param_target_label(uint8_t device_idx, uint8_t target,
                                  char *out, uint8_t len) const override;
  virtual bool param_label(uint8_t device_idx, uint8_t target,
                           uint8_t param, char *out,
                           uint8_t len) override;
  virtual bool get_param(uint8_t device_idx, uint8_t target, uint8_t param,
                         uint8_t *value) override;
  virtual bool set_param(uint8_t device_idx, uint8_t target, uint8_t param,
                         uint8_t value,
                         MidiUartClass *uart_ = nullptr) override;
  virtual void mixer_mute_track(uint8_t device_idx, uint8_t track,
                                bool mute,
                                MidiUartClass *uart_ = nullptr) override;
  virtual void mixer_set_record_mutes(uint8_t device_idx, uint8_t track,
                                      bool state,
                                      bool clear = false) override;
  virtual void ui_loop() override;
  virtual bool handle_ui_event(gui_event_t *event) override;
  virtual bool enter_ui(gui_event_t *event) override;
  virtual bool supports_ui() const override { return true; }
  virtual bool is_ui_active() override;
  virtual bool is_ui_collapsed() override;
  virtual void exit_ui() override;
  virtual void on_ui_slot_button(uint8_t slot, bool pressed) override;
  bool enter_diag_ui(uint8_t device_idx);
  bool select_ui_track(uint8_t track_idx);
  uint8_t ui_device_idx() const { return ui_device_idx_; }
  bool p4_defaults_loaded() const { return p4_defaults_loaded_; }
  bool get_default_p4_sound(uint8_t p4_track_index,
                            TbdP4SoundData *sound) const;
  bool hydrate_p4_sound(TbdP4SoundData &sound);

private:
  uint8_t ui_device_idx_ = 255;
  bool diag_active_ = false;
  bool p4_defaults_loaded_ = false;
  bool p4_defaults_init_in_progress_ = false;
  bool p4_defaults_init_failed_ = false;
  bool grid_devices_initialized_[2] = {};
  uint32_t p4_defaults_last_attempt_ms_ = 0;
  uint8_t active_note_ = 255;
  uint8_t active_note_channel_ = 0;

  bool load_default_p4_presets(bool show_progress = false);
  void apply_runtime_p4_defaults();
  void sync_active_p4_track();
  void note_on(uint8_t note);
  void note_off();
};

extern TbdDevice TBD;

bool tbd_get_default_p4_sound(uint8_t p4_track_index,
                              TbdP4SoundData *sound);
bool tbd_hydrate_p4_sound(TbdP4SoundData &sound);
void tbd_p4_send_param_value(MidiUartClass *uart, uint8_t midi_channel,
                             const TbdP4ParamDescriptor &param,
                             int16_t value);
void tbd_p4_send_sound_state(const TbdP4SoundData &sound);
void tbd_p4_send_sound_mixer_state(const TbdP4SoundData &sound);
uint8_t tbd_p4_driver_param_page_count();
TbdP4ParamDescriptor *tbd_p4_driver_param(uint8_t index);
void tbd_p4_send_driver_param(uint8_t index);
void tbd_p4_send_driver_params();

#endif // PLATFORM_TBD
