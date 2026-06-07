#include "MidiDevice.h"

#include "Midi.h"
#include "MidiID.h"
#include "MidiUart.h"
#include "MCLSeq.h"
#include "Project.h"
#include "ResourceManager.h"

MidiDevice::MidiDevice(MidiClass *_midi, const char *_name, const uint8_t _id,
                       const bool _isElektronDevice, const char *_full_name)
    : name(_name), full_name(_full_name != nullptr ? _full_name : _name),
      id(_id), isElektronDevice(_isElektronDevice)
#ifdef MCL_HAS_DEVICE_CAPABILITIES
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

#if defined(__AVR__)
void MidiDevice::init_ext_grid_devices(DeviceIdx device_idx,
                                       uint8_t first_track_type,
                                       uint8_t first_count,
                                       uint8_t rest_track_type) {
  GridDeviceTrack gdt;
  for (uint8_t i = 0; i < NUM_EXT_TRACKS; i++) {
    gdt.init(i < first_count ? first_track_type : rest_track_type, GROUP_DEV,
             static_cast<uint8_t>(device_idx), &mcl_seq.ext_tracks[i]);
    add_track_to_grid(GridIdx::Y, i, &gdt);
  }
}
#endif

void MidiDevice::cleanup(DeviceIdx device_idx) {
  for (uint8_t n = 0; n < NUM_GRIDS; n++) {
    proj.grids[n].cleanup(static_cast<uint8_t>(device_idx));
  }
}

DeviceMixerCapability *MidiDevice::mixer() {
  return nullptr;
}

#ifdef MCL_HAS_DEVICE_CAPABILITIES
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

DevicePanelCapability *MidiDevice::panel() {
  return &panel_capability_;
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

MidiDeviceLogoGif MidiDevice::logo_gif() const {
#ifdef PLATFORM_TBD
  if (port == UARTP4_PORT) {
    return {nullptr, nullptr};
  }
#endif
  switch (id) {
  case DEVICE_MD:
    return {R.icons_logo->machinedrum_gif,
            R.icons_logo->machinedrum_gif_data};
  case DEVICE_MNM:
    return {R.icons_logo->monomachine_gif,
            R.icons_logo->monomachine_gif_data};
  case DEVICE_A4:
    return {R.icons_logo->analog_gif, R.icons_logo->analog_gif_data};
  default:
    return {R.icons_logo->midi_gif, R.icons_logo->midi_gif_data};
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
