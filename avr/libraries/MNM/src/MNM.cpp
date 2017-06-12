#include "MNM.h"

MNMClass::MNMClass() {
  currentGlobal = -1;
  currentKit = -1;
  currentPattern = -1;
  global.baseChannel = 0;
  loadedKit = loadedGlobal = false;
  currentTrack = 0;
}

void MNMClass::sendNoteOn(uint8_t track, uint8_t note, uint8_t velocity) {
  MidiUart.sendNoteOn(track + global.baseChannel, note, velocity);
}

void MNMClass::sendNoteOff(uint8_t track, uint8_t note) {
  MidiUart.sendNoteOff(track + global.baseChannel, note);
}

void MNMClass::sendMultiTrigNoteOn(uint8_t note, uint8_t velocity) {
  MidiUart.sendNoteOn(global.multitrigChannel, note, velocity);
}

void MNMClass::sendMultiTrigNoteOff(uint8_t note) {
  MidiUart.sendNoteOff(global.multitrigChannel, note);
}

void MNMClass::sendMultiMapNoteOn(uint8_t note, uint8_t velocity) {
  MidiUart.sendNoteOn(global.multimapChannel, note, velocity);
}

void MNMClass::sendMultiMapNoteOff(uint8_t note) {
  MidiUart.sendNoteOff(global.multimapChannel, note);
}

void MNMClass::sendAutoNoteOn(uint8_t note, uint8_t velocity) {
  MidiUart.sendNoteOn(global.autotrackChannel, note, velocity);
}

void MNMClass::sendAutoNoteOff(uint8_t note) {
  MidiUart.sendNoteOff(global.autotrackChannel, note);
}

void MNMClass::triggerTrack(uint8_t track, bool amp, bool lfo, bool filter) {
  MidiUart.sendNRPN(global.baseChannel,
		    (uint16_t)(0x7F << 7),
		    (uint8_t)((track << 3) | (amp ? 4 : 0) | (lfo ? 2 : 0) | (filter ? 1 : 0)));
}

void MNMClass::setMultiEnvParam(uint8_t param, uint8_t value) {
  MidiUart.sendNRPN(global.baseChannel, 0x40 + param, value);
}

void MNMClass::setMute(uint8_t track, bool mute) {
  MidiUart.sendCC(track + global.baseChannel, 3, mute ? 0 : 1);
}

void MNMClass::setAutoMute(bool mute) {
  MidiUart.sendCC(global.autotrackChannel, 3, mute ? 0 : 1);
}

void MNMClass::setMidiParam(uint8_t track, uint8_t param, uint8_t value) {
  MidiUart.sendNRPN(global.baseChannel, (track << 7) | (0x38 + param), value);
}

void MNMClass::setTrackPitch(uint8_t track, uint8_t pitch) {
  MidiUart.sendNRPN(global.baseChannel, (112 + track) << 7, pitch);
}

void MNMClass::setTrackLevel(uint8_t track, uint8_t level) {
  MidiUart.sendCC(global.baseChannel + track, 7, level);
}

void MNMClass::setAutoParam(uint8_t param, uint8_t value) {
  if (param < 0x30) {
    MidiUart.sendCC(global.autotrackChannel, param + 0x30, value);
  } else {
    MidiUart.sendCC(global.autotrackChannel, param + 0x38, value);
  }
}

void MNMClass::setAutoLevel(uint8_t level) {
  MidiUart.sendCC(global.autotrackChannel, 7, level);
}

void MNMClass::setParam(uint8_t track, uint8_t param, uint8_t value) {
  uint8_t cc = 0;
  if (param == 100) {
    cc = 0x3; // MUT
  } else if (param == 101) {
    cc = 0x7; // LEV;
  } else if (param < 0x10) {
    cc = param + 0x30;
  } else if (param < 0x28) {
    cc = param + 0x38;
  } else {
    cc = param + 0x40;
  }
  MidiUart.sendCC(global.baseChannel + track, cc, value);
}

bool MNMClass::parseCC(uint8_t channel, uint8_t cc, uint8_t *track, uint8_t *param) {
  if ((channel >= global.baseChannel) && (channel < (global.baseChannel + 6))) {
    *track = channel - global.baseChannel;
    if ((cc >= 0x30) && (cc <= 0x3f)) {
      *param = cc - 0x30;
      return true;
    } else if ((cc >= 0x48) && (cc <= 0x5f)) {
      *param = cc - 0x38;
      return true;
    } else if ((cc >= 0x68) && (cc <= 0x77)) {
      *param = cc - 0x40;
      return true;
    } else if (cc == 0x3) {
      *param = 100; // MUTE
      return true;
    } else if (cc == 0x07) {
      *param = 101; // LEV
      return true;
    }
  }

  return false;
}

void MNMClass::sendSysex(uint8_t *bytes, uint8_t cnt) {
  MidiUart.putc(0xF0);
  MidiUart.sendRaw(monomachine_sysex_hdr, sizeof(monomachine_sysex_hdr));
  MidiUart.sendRaw(bytes, cnt);
  MidiUart.putc(0xF7);
}

void MNMClass::setStatus(uint8_t id, uint8_t value) {
  uint8_t data[] = { 0x71, id & 0x7F, value & 0x7F };
  MNM.sendSysex(data, countof(data));
}

void MNMClass::loadGlobal(uint8_t id) {
  setStatus(1, id);
}

void MNMClass::loadKit(uint8_t id) {
  setStatus(2, id);
}

void MNMClass::loadPattern(uint8_t id) {
  setStatus(4, id);
}

void MNMClass::loadSong(uint8_t id) {
  setStatus(8, id);
}

void MNMClass::setSequencerMode(bool songMode) {
  setStatus(0x10, songMode ? 1 : 0);
}

void MNMClass::setAudioMode(bool polyMode) {
  setStatus(0x20, polyMode ? 1 : 0);
}

void MNMClass::setSequencerModeMode(bool midiMode) {
  setStatus(0x21, midiMode ? 1 : 0);
}

void MNMClass::setAudioTrack(uint8_t track) {
  setStatus(0x22, track);
}

void MNMClass::setMidiTrack(uint8_t track) {
  setStatus(0x23, track);
}

void MNMClass::setCurrentKitName(char *name) {
  // XXX
}

void MNMClass::saveCurrentKit(uint8_t id) {
  // XXX
}

void MNMClass::sendRequest(uint8_t type, uint8_t param) {
  uint8_t data[] = { type, param };
  MNM.sendSysex(data, countof(data));
}

void MNMClass::revertToCurrentKit(bool reloadKit) {
  if (!reloadKit) {
    if (loadedKit) {
      MNM.loadKit(MNM.currentKit);
    }
  } else {
    uint8_t kit = getCurrentKit(500);
    if (kit != 255) {
      MNM.loadKit(MNM.currentKit);
    }
  }
}


void MNMClass::revertToTrack(uint8_t track, bool reloadKit) {
  if (!reloadKit) {
    if (loadedKit) {
      setMachine(track, track);
    }
  }
}

void MNMClass::requestKit(uint8_t _kit) {
  sendRequest(MNM_KIT_REQUEST_ID, _kit);
}

void MNMClass::requestPattern(uint8_t _pattern) {
  sendRequest(MNM_PATTERN_REQUEST_ID, _pattern);
}

void MNMClass::requestSong(uint8_t _song) {
  sendRequest(MNM_SONG_REQUEST_ID, _song);
}

void MNMClass::requestGlobal(uint8_t _global) {
  sendRequest(MNM_GLOBAL_REQUEST_ID, _global);
}

void MNMClass::assignMachine(uint8_t track, uint8_t model, bool initAll, bool initSynth) {
  uint8_t data[] = { MNM_LOAD_GLOBAL_ID, track, model, 0x00 };
  if (initAll) {
    data[3] = 0x01;
  } else if (initSynth) {
    data[3] = 0x02;
  } else {
    data[3] = 0x00;
  }
  MNM.sendSysex(data, countof(data));
}

void MNMClass::setMachine(uint8_t track, uint8_t idx) {
  assignMachine(track, kit.models[idx]);
  for (int i = 0; i < 56; i++) {
    setParam(track, i, kit.parameters[idx][i]);
  }
  setTrackLevel(track, kit.levels[idx]);
}

class BlockCurrentStatusCallback : public MNMCallback {
public:
  uint8_t type;
  uint8_t value;
  bool received;

  BlockCurrentStatusCallback(uint8_t _type) {
    type = _type;
    received = false;
    value = 255;
  }

  void onStatusResponseCallback(uint8_t _type, uint8_t param) {
    if (type == _type) {
      value = param;
      received = true;
    }
  }
};

uint8_t MNMClass::getBlockingStatus(uint8_t type, uint16_t timeout) {
  uint16_t start_clock = read_slowclock();
  uint16_t current_clock = start_clock;;
  BlockCurrentStatusCallback cb(type);

  MNMSysexListener.addOnStatusResponseCallback
    (&cb, (mnm_status_callback_ptr_t)&BlockCurrentStatusCallback::onStatusResponseCallback);
  MNM.sendRequest(MNM_STATUS_REQUEST_ID, type);
  do {
    current_clock = read_slowclock();
    handleIncomingMidi();
  } while ((clock_diff(start_clock, current_clock) < timeout) && !cb.received);
  MNMSysexListener.removeOnStatusResponseCallback(&cb);

  return cb.value;
}
  

uint8_t MNMClass::getCurrentTrack(uint16_t timeout) {
  uint8_t value = getBlockingStatus(MNM_CURRENT_AUDIO_TRACK_REQUEST, timeout);
  if (value == 255) {
    return 255;
  } else {
    MNM.currentTrack = value;
    return value;
  }
}

uint8_t MNMClass::getCurrentKit(uint16_t timeout) {
  uint8_t value = getBlockingStatus(MNM_CURRENT_KIT_REQUEST, timeout);
  if (value == 255) {
    return 255;
  } else {
    MNM.currentKit = value;
    return value;
  }
}

MNMClass MNM;
