/* Copyright (c) 2009 - http://ruinwesen.com/ */

#include "WProgram.h"

#include <Midi.h>
#include <MidiUartParent.hh>
#include "Merger.h"

#ifndef HOST_MIDIDUINO

void MergerSysexListener::end() {
  MidiUart.sendCommandByte(0xF0);
  MidiUart.puts(MidiSysex2.data, MidiSysex2.recordLen);
  MidiUart.putc(0xF7);
}

void Merger::setMergeMask(uint8_t _mask) {
  mask = _mask;

  if (mask & MERGE_CC_MASK) {
    Midi2.addOnControlChangeCallback(this, (midi_callback_ptr_t)&Merger::on3ByteCallback);
  } else {
    Midi2.removeOnControlChangeCallback(this, (midi_callback_ptr_t)&Merger::on3ByteCallback);
  }
  if (mask & MERGE_NOTE_MASK) {
    Midi2.addOnNoteOnCallback(this, (midi_callback_ptr_t)&Merger::on3ByteCallback);
    Midi2.addOnNoteOffCallback(this, (midi_callback_ptr_t)&Merger::on3ByteCallback);
  } else {
    Midi2.removeOnNoteOnCallback(this, (midi_callback_ptr_t)&Merger::on3ByteCallback);
    Midi2.removeOnNoteOffCallback(this, (midi_callback_ptr_t)&Merger::on3ByteCallback);
  }
  if (mask & MERGE_AT_MASK) {
    Midi2.addOnAfterTouchCallback(this, (midi_callback_ptr_t)&Merger::on3ByteCallback);
  } else {
    Midi2.removeOnAfterTouchCallback(this, (midi_callback_ptr_t)&Merger::on3ByteCallback);
  }
  if (mask & MERGE_PRGCHG_MASK) {
    Midi2.addOnProgramChangeCallback(this, (midi_callback_ptr_t)&Merger::on2ByteCallback);
  } else {
    Midi2.removeOnProgramChangeCallback(this, (midi_callback_ptr_t)&Merger::on2ByteCallback);
  }
  if (mask & MERGE_CHANPRESS_MASK) {
    Midi2.addOnChannelPressureCallback(this, (midi_callback_ptr_t)&Merger::on2ByteCallback);
  } else {
    Midi2.removeOnChannelPressureCallback(this, (midi_callback_ptr_t)&Merger::on2ByteCallback);
  }
  if (mask & MERGE_PITCH_MASK) {
    Midi2.addOnPitchWheelCallback(this, (midi_callback_ptr_t)&Merger::on3ByteCallback);
  } else {
    Midi2.removeOnPitchWheelCallback(this, (midi_callback_ptr_t)&Merger::on3ByteCallback);
  }
  if (mask & MERGE_SYSEX_MASK) {
    MidiSysex2.addSysexListener(&mergerSysexListener);
  } else {
    MidiSysex2.removeSysexListener(&mergerSysexListener);
  }
}

void Merger::on2ByteCallback(uint8_t *msg) {
  MidiUart.sendMessage(msg[0], msg[1]);
}

void Merger::on3ByteCallback(uint8_t *msg) {
  MidiUart.sendMessage(msg[0], msg[1], msg[2]);
}

#endif
