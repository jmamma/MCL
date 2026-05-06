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
  virtual void on_connection(uint8_t device_idx) override;
  virtual void init_grid_devices(uint8_t device_idx) override;
  virtual bool supports_capability(MidiDeviceCapability capability) const override;
  virtual void muteTrack(uint8_t track, bool mute = true,
                         MidiUartClass *uart_ = nullptr) override;
  virtual uint8_t mixer_default_param(uint8_t device_idx) const override;
  virtual bool mixer_param(uint8_t device_idx, uint8_t track,
                           uint8_t param_idx,
                           MidiDeviceMixerParam *param) override;
  virtual bool set_mixer_param(uint8_t device_idx, uint8_t track,
                               uint8_t param_idx, int16_t value,
                               bool send = true) override;
  virtual void mixer_mute_track(uint8_t device_idx, uint8_t track,
                                bool mute,
                                MidiUartClass *uart_ = nullptr) override;
  virtual void mixer_set_record_mutes(uint8_t device_idx, uint8_t track,
                                      bool state,
                                      bool clear = false) override;
  virtual void ui_loop() override;
  virtual bool handle_ui_event(gui_event_t *event) override;
  virtual bool enter_ui(gui_event_t *event) override;
  virtual bool is_ui_active() override;
  virtual void exit_ui() override;
  uint8_t ui_device_idx() const { return ui_device_idx_; }
  bool get_default_p4_sound(uint8_t p4_track_index,
                            TbdP4SoundData *sound) const;
  bool hydrate_p4_sound(TbdP4SoundData &sound);

private:
  uint8_t ui_device_idx_ = 255;
  bool diag_active_ = false;
  bool p4_defaults_loaded_ = false;
  uint32_t p4_defaults_last_attempt_ms_ = 0;
  uint8_t active_note_ = 255;

  bool load_default_p4_presets();
  void note_on(uint8_t note);
  void note_off();
};

extern TbdDevice TBD;

bool tbd_get_default_p4_sound(uint8_t p4_track_index,
                              TbdP4SoundData *sound);
bool tbd_hydrate_p4_sound(TbdP4SoundData &sound);

#endif // PLATFORM_TBD
