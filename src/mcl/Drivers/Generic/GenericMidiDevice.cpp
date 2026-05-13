#include "GenericMidiDevice.h"

#include "MCLGUI.h"
#include "MCLSeq.h"
#include "MCLSysConfig.h"
#include "ResourceManager.h"
#include "TurboLight.h"
#include <string.h>

class GenericMidiMixerCapability : public ExtMixerCapability {
public:
  explicit GenericMidiMixerCapability(GenericMidiDevice &device)
      : ExtMixerCapability(device, device.mixer_levels, true) {}

protected:
  void send_level(uint8_t track, uint8_t level, bool send) override {
    if (send) {
      static_cast<GenericMidiDevice &>(device_).setLevel(track, level);
    }
  }
};

#if !defined(__AVR__)
class GenericMidiParamCapability : public DeviceParamCapability {
public:
  explicit GenericMidiParamCapability(GenericMidiDevice &device)
      : DeviceParamCapability(device) {}
  virtual uint8_t target_count(const DeviceContext &ctx) const override;
  virtual uint8_t param_count(const DeviceContext &ctx,
                              uint8_t target) const override;
  virtual bool set_param(const DeviceContext &ctx, uint8_t target,
                         uint8_t param, uint8_t value,
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

#if !defined(__AVR__)
DeviceParamCapability *GenericMidiDevice::params() {
  static GenericMidiParamCapability capability(*this);
  return &capability;
}

uint8_t
GenericMidiParamCapability::target_count(const DeviceContext &ctx) const {
  (void)ctx;
  return NUM_EXT_TRACKS;
}

uint8_t GenericMidiParamCapability::param_count(const DeviceContext &ctx,
                                                uint8_t target) const {
  (void)ctx;
  return target < NUM_EXT_TRACKS ? 128 : 0;
}

bool GenericMidiParamCapability::set_param(const DeviceContext &ctx,
                                           uint8_t target, uint8_t param,
                                           uint8_t value,
                                           MidiUartClass *uart_) {
  (void)ctx;
  if (target >= NUM_EXT_TRACKS || param >= 128) {
    return false;
  }
  mcl_seq.ext_tracks[target].send_cc(param, value, uart_);
  return true;
}
#endif

void GenericMidiDevice::init_grid_devices(DeviceIdx device_idx) {
  GridDeviceTrack gdt;

  for (uint8_t i = 0; i < NUM_EXT_TRACKS; i++) {
    gdt.init(EXT_TRACK_TYPE, GROUP_DEV, static_cast<uint8_t>(device_idx),
             &(mcl_seq.ext_tracks[i]));
    add_track_to_grid(GridIdx::Y, i, &gdt);
  }
}
