#include "GenericMidiDevice.h"

#include "MCLGUI.h"
#include "MCLSeq.h"
#include "MCLSysConfig.h"
#include "ResourceManager.h"
#include "TurboLight.h"
#include <string.h>

class GenericMidiMixerCapability : public DeviceMixerCapability {
public:
  explicit GenericMidiMixerCapability(GenericMidiDevice &device)
      : DeviceMixerCapability(device) {}
  virtual bool param(uint8_t device_idx, uint8_t track, uint8_t param_idx,
                     MidiDeviceMixerParam *param) override;
  virtual bool set_param(uint8_t device_idx, uint8_t track,
                         uint8_t param_idx, int16_t value,
                         bool send = true) override;
  virtual void set_record_mutes(uint8_t device_idx, uint8_t track,
                                bool state, bool clear = false) override;
  virtual bool parse_cc(uint8_t device_idx, uint8_t channel, uint8_t cc,
                        uint8_t *track, uint8_t *param) const override;
  virtual void update_from_cc(uint8_t device_idx, uint8_t track,
                              uint8_t param, int16_t value) override;

private:
  GenericMidiDevice &generic() const { return (GenericMidiDevice &)device_; }
};

#if !defined(__AVR__)
class GenericMidiParamCapability : public DeviceParamCapability {
public:
  explicit GenericMidiParamCapability(GenericMidiDevice &device)
      : DeviceParamCapability(device) {}
  virtual uint8_t target_count(uint8_t device_idx) const override;
  virtual uint8_t param_count(uint8_t device_idx,
                              uint8_t target) const override;
  virtual bool set_param(uint8_t device_idx, uint8_t target, uint8_t param,
                         uint8_t value,
                         MidiUartClass *uart_ = nullptr) override;
};
#endif

GenericMidiDevice::GenericMidiDevice()
    : MidiDevice(&Midi2, "MI", DEVICE_MIDI, false) {
  memset(mixer_levels, 127, sizeof(mixer_levels));
}

bool GenericMidiDevice::probe() {
  if (mcl_cfg.uart2_turbo_speed) {
    mcl_gui.delay_progress(1200);
    connected = true;
    turbo_light.set_speed(turbo_light.lookup_speed(mcl_cfg.uart2_turbo_speed),
                          uart);
  }
  return true;
}

uint8_t GenericMidiDevice::get_mute_cc() {
  return mcl_cfg.uart2_cc_mute > 127 ? 255 : mcl_cfg.uart2_cc_mute;
}

void GenericMidiDevice::muteTrack(uint8_t track, bool mute,
                                  MidiUartClass *uart_) {
  if (track >= NUM_EXT_TRACKS || mcl_cfg.uart2_cc_mute > 127) {
    return;
  }
  if (uart_ == nullptr) {
    uart_ = uart;
  }
  uart_->sendCC(mcl_seq.ext_tracks[track].channel, mcl_cfg.uart2_cc_mute,
                mute ? 127 : 0);
}

void GenericMidiDevice::setLevel(uint8_t track, uint8_t value,
                                 MidiUartClass *uart_) {
  if (track >= NUM_EXT_TRACKS || mcl_cfg.uart2_cc_level > 127) {
    return;
  }
  if (uart_ == nullptr) {
    uart_ = uart;
  }
  uart_->sendCC(mcl_seq.ext_tracks[track].channel, mcl_cfg.uart2_cc_level,
                value);
}

DeviceMixerCapability *GenericMidiDevice::mixer() {
  static GenericMidiMixerCapability capability(*this);
  return &capability;
}

bool GenericMidiMixerCapability::param(uint8_t device_idx, uint8_t track,
                                       uint8_t param_idx,
                                       MidiDeviceMixerParam *param) {
  (void)device_idx;
  if (param == nullptr || track >= NUM_EXT_TRACKS || param_idx != 0 ||
      mcl_cfg.uart2_cc_level > 127) {
    return false;
  }
  GenericMidiDevice &device = generic();
  param->set_value(device.mixer_levels[track]);
  param->set_metadata("LEV", 0, true);
  return true;
}

bool GenericMidiMixerCapability::set_param(uint8_t device_idx, uint8_t track,
                                           uint8_t param_idx, int16_t value,
                                           bool send) {
  (void)device_idx;
  if (track >= NUM_EXT_TRACKS || param_idx != 0 ||
      mcl_cfg.uart2_cc_level > 127) {
    return false;
  }
  if (value < 0) value = 0;
  if (value > 127) value = 127;
  GenericMidiDevice &device = generic();
  device.mixer_levels[track] = (uint8_t)value;
  if (send) {
    device.setLevel(track, device.mixer_levels[track]);
  }
  return true;
}

bool GenericMidiMixerCapability::parse_cc(uint8_t device_idx, uint8_t channel,
                                          uint8_t cc, uint8_t *track,
                                          uint8_t *param) const {
  (void)device_idx;
  if (track == nullptr || param == nullptr) {
    return false;
  }
  *track = mcl_seq.find_ext_track(channel);
  if (*track == 255) {
    return false;
  }
  if (cc == mcl_cfg.uart2_cc_level && mcl_cfg.uart2_cc_level <= 127) {
    *param = default_param(device_idx);
    return true;
  }
  if (cc == generic().get_mute_cc()) {
    *param = MUTE_PARAM;
    return true;
  }
  return false;
}

void GenericMidiMixerCapability::update_from_cc(uint8_t device_idx,
                                                uint8_t track, uint8_t param,
                                                int16_t value) {
  (void)device_idx;
  if (track >= NUM_EXT_TRACKS) {
    return;
  }
  if (is_mute_param(param)) {
    mcl_seq.ext_tracks[track].mute_state =
        value > 0 ? SEQ_MUTE_ON : SEQ_MUTE_OFF;
    return;
  }
  if (param == default_param(device_idx)) {
    if (value < 0) value = 0;
    if (value > 127) value = 127;
    generic().mixer_levels[track] = (uint8_t)value;
  }
}

#if !defined(__AVR__)
DeviceParamCapability *GenericMidiDevice::params() {
  static GenericMidiParamCapability capability(*this);
  return &capability;
}

uint8_t GenericMidiParamCapability::target_count(uint8_t device_idx) const {
  (void)device_idx;
  return NUM_EXT_TRACKS;
}

uint8_t GenericMidiParamCapability::param_count(uint8_t device_idx,
                                                uint8_t target) const {
  (void)device_idx;
  return target < NUM_EXT_TRACKS ? 128 : 0;
}

bool GenericMidiParamCapability::set_param(uint8_t device_idx, uint8_t target,
                                           uint8_t param, uint8_t value,
                                           MidiUartClass *uart_) {
  (void)device_idx;
  if (target >= NUM_EXT_TRACKS || param >= 128) {
    return false;
  }
  mcl_seq.ext_tracks[target].send_cc(param, value, uart_);
  return true;
}
#endif

void GenericMidiMixerCapability::set_record_mutes(uint8_t device_idx,
                                                  uint8_t track, bool state,
                                                  bool clear) {
  (void)device_idx;
  if (track >= NUM_EXT_TRACKS) {
    return;
  }
  mcl_seq.ext_tracks[track].record_mutes = state;
  if (clear) {
    mcl_seq.ext_tracks[track].clear_mute();
  }
}

void GenericMidiDevice::init_grid_devices(uint8_t device_idx) {
  uint8_t grid_idx = 1;
  GridDeviceTrack gdt;

  for (uint8_t i = 0; i < NUM_EXT_TRACKS; i++) {
    gdt.init(EXT_TRACK_TYPE, GROUP_DEV, device_idx, &(mcl_seq.ext_tracks[i]));
    add_track_to_grid(grid_idx, i, &gdt);
  }
}
