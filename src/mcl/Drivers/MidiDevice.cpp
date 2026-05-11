#include "MidiDevice.h"

#include "Midi.h"
#include "MidiID.h"
#include "MidiUart.h"
#include "Project.h"
#include "ResourceManager.h"
#include "SeqTrack.h"

namespace {

#if !defined(__AVR__)
void copy_param_number_label(char prefix, uint8_t number, char *out,
                             uint8_t len) {
  if (out == nullptr || len == 0) {
    return;
  }
  if (len < 2) {
    out[0] = '\0';
    return;
  }

  uint8_t pos = 0;
  out[pos++] = prefix;
  if (number >= 100 && pos + 1 < len) {
    out[pos++] = (char)('0' + number / 100);
    number %= 100;
  }
  if ((number >= 10 || pos > 1) && pos + 1 < len) {
    out[pos++] = (char)('0' + number / 10);
    number %= 10;
  }
  if (pos + 1 < len) {
    out[pos++] = (char)('0' + number);
  }
  out[pos] = '\0';
}
#endif

} // namespace

MidiDevice::MidiDevice(MidiClass *_midi, const char *_name, const uint8_t _id,
                       const bool _isElektronDevice)
    : name(_name), id(_id), isElektronDevice(_isElektronDevice) {
  midi = _midi;
  uart = midi ? midi->uart : nullptr;
  track_type = 0;
  port = 0;
  connected = false;
  in_probe = false;
}

NullMidiDevice::NullMidiDevice()
    : MidiDevice(nullptr, "  ", DEVICE_NULL, false) {}

void MidiDevice::add_track_to_grid(uint8_t grid_idx, uint8_t track_idx,
                                   GridDeviceTrack *gdt) {
  proj.grids[grid_idx].add_track(track_idx, gdt);
}

void MidiDevice::cleanup(uint8_t device_idx) {
  for (uint8_t n = 0; n < NUM_GRIDS; n++) {
    proj.grids[n].cleanup(device_idx);
  }
}

uint8_t MidiDevice::mixer_track_count(uint8_t device_idx) const {
  if (device_idx >= NUM_GRIDS) {
    return 0;
  }
  uint8_t count = 0;
  const MidiDeviceGrid &grid = proj.grids[device_idx];
  for (uint8_t n = 0; n < GRID_WIDTH; n++) {
    const GridDeviceTrack &gdt = grid.tracks[n];
    if (gdt.track_type != EMPTY_TRACK_TYPE && gdt.group_type == GROUP_DEV &&
        gdt.device_idx == device_idx && gdt.seq_track != nullptr) {
      count = n + 1;
    }
  }
  return count;
}

SeqTrack *MidiDevice::mixer_seq_track(uint8_t device_idx, uint8_t track) {
  if (device_idx >= NUM_GRIDS || track >= GRID_WIDTH) {
    return nullptr;
  }
  GridDeviceTrack &gdt = proj.grids[device_idx].tracks[track];
  if (gdt.track_type == EMPTY_TRACK_TYPE || gdt.group_type != GROUP_DEV ||
      gdt.device_idx != device_idx) {
    return nullptr;
  }
  return gdt.seq_track;
}

uint8_t MidiDevice::mixer_default_param(uint8_t device_idx) const {
  (void)device_idx;
  return 0;
}

bool MidiDevice::mixer_param(uint8_t device_idx, uint8_t track,
                             uint8_t param_idx,
                             MidiDeviceMixerParam *param) {
  (void)device_idx;
  (void)track;
  (void)param_idx;
  (void)param;
  return false;
}

bool MidiDevice::set_mixer_param(uint8_t device_idx, uint8_t track,
                                 uint8_t param_idx, int16_t value,
                                 bool send) {
  (void)device_idx;
  (void)track;
  (void)param_idx;
  (void)value;
  (void)send;
  return false;
}

#if !defined(__AVR__)
uint8_t MidiDevice::param_target_count(uint8_t device_idx) const {
  (void)device_idx;
  return 0;
}

uint8_t MidiDevice::param_count(uint8_t device_idx, uint8_t target) const {
  (void)device_idx;
  (void)target;
  return 0;
}

bool MidiDevice::param_target_label(uint8_t device_idx, uint8_t target,
                                    char *out, uint8_t len) const {
  (void)device_idx;
  (void)target;
  (void)out;
  (void)len;
  return false;
}

bool MidiDevice::param_label(uint8_t device_idx, uint8_t target,
                             uint8_t param, char *out, uint8_t len) {
  (void)device_idx;
  (void)target;
  (void)param;
  (void)out;
  (void)len;
  return false;
}

bool MidiDevice::get_param(uint8_t device_idx, uint8_t target, uint8_t param,
                           uint8_t *value) {
  (void)device_idx;
  (void)target;
  (void)param;
  (void)value;
  return false;
}

bool MidiDevice::set_param(uint8_t device_idx, uint8_t target, uint8_t param,
                           uint8_t value, MidiUartClass *uart_) {
  (void)device_idx;
  (void)target;
  (void)param;
  (void)value;
  (void)uart_;
  return false;
}

uint8_t MidiDevice::sequencer_lock_param_count(uint8_t device_idx,
                                               uint8_t target) const {
  return param_count(device_idx, target);
}

bool MidiDevice::sequencer_lock_param_info(uint8_t device_idx, uint8_t target,
                                           uint8_t param,
                                           MidiDeviceParamInfo *info) {
  if (info == nullptr || param >= sequencer_lock_param_count(device_idx,
                                                            target)) {
    return false;
  }
  *info = MidiDeviceParamInfo();
  info->active = true;
  info->sendable = true;
  info->param_id = param;
  info->ctrl = param;
  uint8_t value = 0;
  if (get_param(device_idx, target, param, &value)) {
    info->default_value = value;
    info->current_value = value;
  }
  return true;
}

bool MidiDevice::sequencer_lock_param_label(uint8_t device_idx,
                                            uint8_t target, uint8_t param,
                                            char *out, uint8_t len) {
  if (out == nullptr || len == 0 ||
      param >= sequencer_lock_param_count(device_idx, target)) {
    return false;
  }
  if (param_label(device_idx, target, param, out, len)) {
    return true;
  }
  copy_param_number_label('P', param, out, len);
  return true;
}

bool MidiDevice::sequencer_uses_step_pitch(uint8_t device_idx,
                                           uint8_t target) const {
  (void)device_idx;
  (void)target;
  return false;
}

uint8_t MidiDevice::sequencer_pitch_lock_param(uint8_t device_idx,
                                               uint8_t target) const {
  (void)device_idx;
  (void)target;
  return 0;
}
#endif

void MidiDevice::mixer_mute_track(uint8_t device_idx, uint8_t track,
                                  bool mute, MidiUartClass *uart_) {
  (void)device_idx;
  muteTrack(track, mute, uart_);
}

void MidiDevice::mixer_set_record_mutes(uint8_t device_idx, uint8_t track,
                                        bool state, bool clear) {
  (void)clear;
  SeqTrack *seq_track = mixer_seq_track(device_idx, track);
  if (seq_track != nullptr) {
    seq_track->record_mutes = state;
  }
}

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
