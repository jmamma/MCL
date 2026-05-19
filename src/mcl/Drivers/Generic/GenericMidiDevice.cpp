#include "GenericMidiDevice.h"

#include "GenericMidiTrackRef.h"
#include "MidiSeqExtStepTrackCapability.h"
#include "MCLGUI.h"
#include "MCLSysConfig.h"
#include "ResourceManager.h"
#include "TurboLight.h"
#include <string.h>

namespace {

uint8_t generic_midi_channel(uint8_t track) {
  return GenericMidiTrackRef::channel(track);
}

SeqTrack *generic_midi_seq_track(uint8_t track) {
  return GenericMidiTrackRef::seq_track(track);
}

void generic_midi_set_record_mute(uint8_t track, bool state, bool clear) {
  SeqTrack *seq_track = generic_midi_seq_track(track);
  if (seq_track == nullptr) {
    return;
  }
  seq_track->record_mutes = state;
  if (clear) {
    GenericMidiTrackRef::clear_mute(track);
  }
}

bool generic_midi_track_for_channel(uint8_t channel, uint8_t *track_index) {
  if (track_index == nullptr) {
    return false;
  }
  for (uint8_t i = 0; i < NUM_EXT_TRACKS; i++) {
    if (generic_midi_channel(i) == channel) {
      *track_index = i;
      return true;
    }
  }
  *track_index = 255;
  return false;
}

} // namespace

class GenericMidiMixerCapability final : public ExtMixerCapability {
public:
  explicit GenericMidiMixerCapability(GenericMidiDevice &device)
      : ExtMixerCapability(device, device.mixer_levels, true) {}

  virtual uint8_t track_count(const DeviceContext &ctx) const override {
    (void)ctx;
    return NUM_EXT_TRACKS;
  }

  virtual SeqTrack *seq_track(const DeviceContext &ctx,
                              uint8_t track) override {
    (void)ctx;
    return generic_midi_seq_track(track);
  }

  virtual void set_record_mutes(const DeviceContext &ctx, uint8_t track,
                                bool state, bool clear = false) override {
    (void)ctx;
    generic_midi_set_record_mute(track, state, clear);
  }

  virtual bool parse_cc(const DeviceContext &ctx, uint8_t channel, uint8_t cc,
                        uint8_t *track, uint8_t *param) const override {
    (void)ctx;
    if (!generic_midi_track_for_channel(channel, track) || param == nullptr) {
      return false;
    }
    if (cc == mcl_cfg.uart2_cc_level && mcl_cfg.uart2_cc_level <= 127) {
      *param = 0;
      return true;
    }
    if (cc == device_.get_mute_cc()) {
      *param = DeviceMixerCapability::MUTE_PARAM;
      return true;
    }
    return false;
  }

  virtual void update_from_cc(const DeviceContext &ctx, uint8_t track,
                              uint8_t param,
                              MidiDeviceMixerValue value) override {
    (void)ctx;
    if (track >= NUM_EXT_TRACKS) {
      return;
    }
    if (param == DeviceMixerCapability::MUTE_PARAM) {
      SeqTrack *seq_track = generic_midi_seq_track(track);
      if (seq_track != nullptr) {
        seq_track->mute_state = value > 0 ? SEQ_MUTE_ON : SEQ_MUTE_OFF;
      }
      return;
    }
    if (param == 0) {
      if (value < 0) value = 0;
      if (value > 127) value = 127;
      static_cast<GenericMidiDevice &>(device_).mixer_levels[track] =
          (uint8_t)value;
    }
  }

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
                         MidiUartClass *uart_ = nullptr,
                         bool update_kit = false) override;
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
  uart_->sendCC(generic_midi_channel(track), mcl_cfg.uart2_cc_mute,
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
  uart_->sendCC(generic_midi_channel(track), mcl_cfg.uart2_cc_level,
                value);
}

DeviceMixerCapability *GenericMidiDevice::mixer() {
  static GenericMidiMixerCapability capability(*this);
  return &capability;
}

#if !defined(__AVR__)
DeviceExtStepTrackCapability *GenericMidiDevice::ext_step_tracks() {
  static MidiSeqExtStepTrackCapability capability(*this);
  return &capability;
}

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
                                           MidiUartClass *uart_,
                                           bool update_kit) {
  (void)ctx;
  (void)update_kit;
  if (target >= NUM_EXT_TRACKS || param >= 128) {
    return false;
  }
  GenericMidiTrackRef::send_cc(target, param, value, uart_);
  return true;
}
#endif

void GenericMidiDevice::init_grid_devices(DeviceIdx device_idx) {
  GridDeviceTrack gdt;

  for (uint8_t i = 0; i < NUM_EXT_TRACKS; i++) {
    GenericMidiTrackRef::init_runtime_track(i);
    gdt.init(GenericMidiTrackRef::grid_track_type(), GROUP_DEV,
             static_cast<uint8_t>(device_idx),
             GenericMidiTrackRef::seq_track(i));
    add_track_to_grid(GridIdx::Y, i, &gdt);
  }
}
