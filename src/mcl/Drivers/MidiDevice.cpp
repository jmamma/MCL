#include "MidiDevice.h"

#include "Midi.h"
#include "MidiID.h"
#include "MidiUart.h"
#include "Project.h"
#include "ResourceManager.h"

MidiDevice::MidiDevice(MidiClass *_midi, const char *_name, const uint8_t _id,
                       const bool _isElektronDevice)
    : name(_name), id(_id), isElektronDevice(_isElektronDevice),
      mixer_capability_(*this)
#if !defined(__AVR__)
      ,
      step_track_capability_(*this),
      ext_step_track_capability_(*this),
      step_edit_capability_(*this), param_capability_(*this),
      perf_capability_(*this)
#endif
{
  midi = _midi;
  uart = midi ? midi->uart : nullptr;
  track_type = 0;
  port = 0;
  connected = false;
  in_probe = false;
}

NullMidiDevice::NullMidiDevice()
    : MidiDevice(nullptr, "  ", DEVICE_NULL, false) {}

void MidiDevice::add_track_to_grid(GridIdx grid_idx, uint8_t track_idx,
                                   GridDeviceTrack *gdt) {
  proj.grids[static_cast<uint8_t>(grid_idx)].add_track(track_idx, gdt);
}

void MidiDevice::cleanup(DeviceIdx device_idx) {
  for (uint8_t n = 0; n < NUM_GRIDS; n++) {
    proj.grids[n].cleanup(static_cast<uint8_t>(device_idx));
  }
}

DeviceMixerCapability *MidiDevice::mixer() {
  return &mixer_capability_;
}

#if !defined(__AVR__)
DeviceStepTrackCapability *MidiDevice::step_tracks() {
  return &step_track_capability_;
}

DeviceExtStepTrackCapability *MidiDevice::ext_step_tracks() {
  return &ext_step_track_capability_;
}

DeviceStepEditCapability *MidiDevice::step_edit() {
  return &step_edit_capability_;
}

DeviceParamCapability *MidiDevice::params() {
  return &param_capability_;
}

DevicePerfCapability *MidiDevice::perf() {
  return &perf_capability_;
}
#endif

#if !defined(__AVR__)
DevicePanelCapability *MidiDevice::panel() {
  static DevicePanelCapability panel_capability;
  return &panel_capability;
}
#endif

void MidiDevice::setPort(MidiClass *_midi, uint8_t _port) {
  cleanup_listeners();
  midi = _midi;
  uart = _midi ? _midi->uart : nullptr;
  port = _port;
  setup_listeners();
}

uint8_t *MidiDevice::icon() const {
#ifdef PLATFORM_TBD
  if (port == UARTP4_PORT) {
    return nullptr;
  }
#endif
  switch (id) {
  case DEVICE_MD:
    return R.icons_device->icon_md;
  case DEVICE_MNM:
    return R.icons_device->icon_mnm;
  case DEVICE_A4:
    return R.icons_device->icon_a4;
  case DEVICE_MIDI:
    return R.icons_device->icon_turbo;
  default:
    return nullptr;
  }
}

MCLGIF *MidiDevice::gif() const {
#ifdef PLATFORM_TBD
  if (port == UARTP4_PORT) {
    return nullptr;
  }
#endif
  switch (id) {
  case DEVICE_MD:
    return R.icons_logo->machinedrum_gif;
  case DEVICE_MNM:
    return R.icons_logo->monomachine_gif;
  case DEVICE_A4:
    return R.icons_logo->analog_gif;
  default:
    return R.icons_logo->midi_gif;
  }
}

uint8_t *MidiDevice::gif_data() const {
#ifdef PLATFORM_TBD
  if (port == UARTP4_PORT) {
    return nullptr;
  }
#endif
  switch (id) {
  case DEVICE_MD:
    return R.icons_logo->machinedrum_gif_data;
  case DEVICE_MNM:
    return R.icons_logo->monomachine_gif_data;
  case DEVICE_A4:
    return R.icons_logo->analog_gif_data;
  default:
    return R.icons_logo->midi_gif_data;
  }
}

void MidiDevice::sendNoteOff(uint8_t channel, uint8_t note,
                             MidiUartClass *uart_) {
  if (!connected) { return; }
  uart_ = uart_ ? uart_ : uart;
  uart_->sendNoteOff(channel, note);
}

void MidiDevice::sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity,
                            MidiUartClass *uart_) {
  if (!connected) { return; }
  uart_ = uart_ ? uart_ : uart;
  uart_->sendNoteOn(channel, note, velocity);
}

void MidiDevice::sendCC(uint8_t channel, uint8_t cc, uint8_t value,
                        MidiUartClass *uart_) {
  if (!connected) { return; }
  uart_ = uart_ ? uart_ : uart;
  uart_->sendCC(channel, cc, value);
}

void MidiDevice::sendPolyKeyPressure(uint8_t channel, uint8_t cc,
                                     uint8_t value, MidiUartClass *uart_) {
  if (!connected) { return; }
  uart_ = uart_ ? uart_ : uart;
  uart_->sendPolyKeyPressure(channel, cc, value);
}

void MidiDevice::sendNRPN(uint8_t channel, uint16_t parameter, uint16_t value,
                          MidiUartClass *uart_) {
  if (!connected) { return; }
  uart_ = uart_ ? uart_ : uart;
  uart_->sendNRPN(channel, parameter, value);
}
