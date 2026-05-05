#include "MidiDevice.h"

#include "Midi.h"
#include "MidiUart.h"
#include "Project.h"
#include "ResourceManager.h"

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

void MidiDevice::add_track_to_grid(uint8_t grid_idx, uint8_t track_idx,
                                   GridDeviceTrack *gdt) {
  proj.grids[grid_idx].add_track(track_idx, gdt);
}

void MidiDevice::cleanup(uint8_t device_idx) {
  for (uint8_t n = 0; n < NUM_GRIDS; n++) {
    proj.grids[n].cleanup(device_idx);
  }
}

void MidiDevice::setPort(MidiClass *_midi, uint8_t _port) {
  cleanup_listeners();
  midi = _midi;
  uart = _midi ? _midi->uart : nullptr;
  port = _port;
  setup_listeners();
}

uint8_t *MidiDevice::gif_data() { return R.icons_logo->midi_gif_data; }
MCLGIF *MidiDevice::gif() { return R.icons_logo->midi_gif; }

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
