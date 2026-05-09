#include "GenericMidiDevice.h"

#include "MCLGUI.h"
#include "MCLSeq.h"
#include "MCLSysConfig.h"
#include "ResourceManager.h"
#include "TurboLight.h"
#include <string.h>

GenericMidiDevice::GenericMidiDevice()
    : MidiDevice(&Midi2, "MI", DEVICE_MIDI, false) {
  memset(mixer_levels, 127, sizeof(mixer_levels));
}

uint8_t *GenericMidiDevice::icon() { return R.icons_device->icon_turbo; }

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

bool GenericMidiDevice::mixer_param(uint8_t device_idx, uint8_t track,
                                    uint8_t param_idx,
                                    MidiDeviceMixerParam *param) {
  (void)device_idx;
  if (param == nullptr || track >= NUM_EXT_TRACKS || param_idx != 0 ||
      mcl_cfg.uart2_cc_level > 127) {
    return false;
  }
  param->set_value(mixer_levels[track]);
  param->set_metadata("LEV", 0, true);
  return true;
}

bool GenericMidiDevice::set_mixer_param(uint8_t device_idx, uint8_t track,
                                        uint8_t param_idx, int16_t value,
                                        bool send) {
  (void)device_idx;
  if (track >= NUM_EXT_TRACKS || param_idx != 0 ||
      mcl_cfg.uart2_cc_level > 127) {
    return false;
  }
  if (value < 0) value = 0;
  if (value > 127) value = 127;
  mixer_levels[track] = (uint8_t)value;
  if (send) {
    setLevel(track, mixer_levels[track]);
  }
  return true;
}

void GenericMidiDevice::mixer_set_record_mutes(uint8_t device_idx,
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
