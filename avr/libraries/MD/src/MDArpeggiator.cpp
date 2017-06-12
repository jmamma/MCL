#include "WProgram.h"
#include "helpers.h"
#include "Arpeggiator.hh"
#include <MD.h>

void MDArpeggiatorClass::recordNote(int pos, uint8_t track, uint8_t note, uint8_t velocity) {
  uint8_t realPitch = MD.trackGetPitch(track, note);
  if (realPitch == 128)
    return;  
  MD.triggerTrack(track, velocity);
  recordPitches[pos] = note;
}
  
void MDArpeggiatorClass::recordNoteSecond(int pos, uint8_t track) {
  pos -= recordLength;
  if (recordPitches[pos + 1] != 128) {
    uint8_t realPitch = MD.trackGetPitch(track, recordPitches[pos + 1]);
    if (realPitch == 128)
      return;  
    MD.setTrackParam(track, 0, realPitch - 5);
#ifndef HOST_MIDIDUINO
    delay(3);
#endif
    MD.setTrackParam(track, 0, realPitch);
  }
}

void MDArpeggiatorClass::playNext(uint32_t _my16thpos, bool recording) {
  int pos = _my16thpos - recordStart;
  if (pos == 0 && recording)
    retrigger();
      
  if (recording && (pos >= (recordLength - 1))) {
    recordNoteSecond(pos, arpTrack);
  }
    
  if (arpLen == 0 || (arpTimes != 0 && arpCount >= arpTimes))
    return;

  if (arpRetrig == RETRIG_BEAT && (_my16thpos % retrigSpeed) == 0)
    retrigger();
  if (++speedCounter >= arpSpeed) {
    speedCounter = 0;
    if (arpStyle == ARP_STYLE_RANDOM) {
      uint8_t i = random(numNotes);
      if (recording && (pos < recordLength)) {
	recordNote(pos, arpTrack, orderedNotes[i] + random(arpOctaves) * 12, orderedVelocities[i]);
      } else if (!recording && !muted) {
	MD.sendNoteOn(arpTrack, orderedNotes[i] + random(arpOctaves) * 12, orderedVelocities[i]);
      }
    } else {
      if (recording && (pos < recordLength)) {
	recordNote(pos, arpTrack, arpNotes[arpStep] + 12 * arpOctaveCount, arpVelocities[arpStep]);
      } else if (!recording && !muted) {
	MD.sendNoteOn(arpTrack, arpNotes[arpStep] + 12 * arpOctaveCount, arpVelocities[arpStep]);
      }
      if (++arpStep == arpLen) {
	if (arpOctaveCount < arpOctaves) {
	  arpStep = 0;
	  arpOctaveCount++;
	} else {
	  arpStep = 0;
	  arpOctaveCount = 0;
	  arpCount++;
	}
      }
    }
  } 
}

void MDArpeggiatorClass::startRecording() {
  triggerRecording = true;
  recording = false;
  endRecording = false;
}  

void MDArpeggiatorClass::on16Callback() {
  if (triggerRecording && (MidiClock.div16th_counter % 16) == 0) {
    triggerRecording = false;
    recording = true;
    recordStart = MidiClock.div16th_counter;
    for (int i = 0; i < 64; i++) {
      recordPitches[i] = 128;
    }
  }
    
  if (recording || endRecording) {
    int pos = MidiClock.div16th_counter - recordStart;
    if (pos >= (recordLength * 3)) {
      endRecording = false;
    } else if (pos >= (recordLength * 2)) {
      recording = false;
      endRecording = true;
      return;
    }
  }
    
  if (!triggerRecording) {
    playNext(recording);
  }
}

void MDArpeggiatorClass::setup() {
  ArpeggiatorClass::setup();
  MidiClock.addOn16Callback(this, (midi_clock_callback_ptr_t)&MDArpeggiatorClass::on16Callback);
}
