#include "MNM.h"
#include "MCL.h"

MNMClass::MNMClass() {
  currentGlobal = -1;
  currentKit = -1;
  currentPattern = -1;
  global.baseChannel = 0;
  loadedKit = loadedGlobal = false;
  currentTrack = 0;
}

void MNMClass::sendNoteOn(uint8_t track, uint8_t note, uint8_t velocity) {
  midiuart->sendNoteOn(track + global.baseChannel, note, velocity);
}

void MNMClass::sendNoteOff(uint8_t track, uint8_t note) {
  midiuart->sendNoteOff(track + global.baseChannel, note);
}

void MNMClass::sendMultiTrigNoteOn(uint8_t note, uint8_t velocity) {
  midiuart->sendNoteOn(global.multitrigChannel, note, velocity);
}

void MNMClass::sendMultiTrigNoteOff(uint8_t note) {
  midiuart->sendNoteOff(global.multitrigChannel, note);
}

void MNMClass::sendMultiMapNoteOn(uint8_t note, uint8_t velocity) {
  midiuart->sendNoteOn(global.multimapChannel, note, velocity);
}

void MNMClass::sendMultiMapNoteOff(uint8_t note) {
  midiuart->sendNoteOff(global.multimapChannel, note);
}

void MNMClass::sendAutoNoteOn(uint8_t note, uint8_t velocity) {
  midiuart->sendNoteOn(global.autotrackChannel, note, velocity);
}

void MNMClass::sendAutoNoteOff(uint8_t note) {
  midiuart->sendNoteOff(global.autotrackChannel, note);
}

void MNMClass::triggerTrack(uint8_t track, bool amp, bool lfo, bool filter) {
  midiuart->sendNRPN(global.baseChannel,
		    (uint16_t)(0x7F << 7),
		    (uint8_t)((track << 3) | (amp ? 4 : 0) | (lfo ? 2 : 0) | (filter ? 1 : 0)));
}

void MNMClass::setMultiEnvParam(uint8_t param, uint8_t value) {
  midiuart->sendNRPN(global.baseChannel, 0x40 + param, value);
}

void MNMClass::setMute(uint8_t track, bool mute) {
  midiuart->sendCC(track + global.baseChannel, 3, mute ? 0 : 1);
}

void MNMClass::setAutoMute(bool mute) {
  midiuart->sendCC(global.autotrackChannel, 3, mute ? 0 : 1);
}

void MNMClass::setMidiParam(uint8_t track, uint8_t param, uint8_t value) {
  midiuart->sendNRPN(global.baseChannel, (track << 7) | (0x38 + param), value);
}

void MNMClass::setTrackPitch(uint8_t track, uint8_t pitch) {
  midiuart->sendNRPN(global.baseChannel, (112 + track) << 7, pitch);
}

void MNMClass::setTrackLevel(uint8_t track, uint8_t level) {
  midiuart->sendCC(global.baseChannel + track, 7, level);
}

void MNMClass::setAutoParam(uint8_t param, uint8_t value) {
  if (param < 0x30) {
    midiuart->sendCC(global.autotrackChannel, param + 0x30, value);
  } else {
    midiuart->sendCC(global.autotrackChannel, param + 0x38, value);
  }
}

void MNMClass::setAutoLevel(uint8_t level) {
  midiuart->sendCC(global.autotrackChannel, 7, level);
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
  midiuart->sendCC(global.baseChannel + track, cc, value);
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
  USE_LOCK();
  SET_LOCK();
  midiuart->m_putc(0xF0);
  midiuart->sendRaw(monomachine_sysex_hdr, sizeof(monomachine_sysex_hdr));
  midiuart->sendRaw(bytes, cnt);
  midiuart->m_putc(0xF7);
  CLEAR_LOCK();
}

void MNMClass::setStatus(uint8_t id, uint8_t value) {
  uint8_t data[] = { 0x71, (uint8_t)(id & 0x7F), (uint8_t)(value & 0x7F) };
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

uint8_t MNMClass::getBlockingStatus(uint8_t type, uint16_t timeout) {
  SysexCallback cb(type);

  MNMSysexListener.addOnStatusResponseCallback
    (&cb, (sysex_status_callback_ptr_t)&SysexCallback::onStatusResponse);
  MNM.sendRequest(MNM_STATUS_REQUEST_ID, type);
  connected = cb.waitBlocking(timeout);
  MNMSysexListener.removeOnStatusResponseCallback(&cb);

  return cb.value;
}
  
bool MNMClass::getBlockingGlobal(uint8_t idx, uint16_t timeout) {
  SysexCallback cb;
  MNMSysexListener.addOnGlobalMessageCallback(&cb, (sysex_callback_ptr_t)&SysexCallback::onSysexReceived);
  MNM.sendRequest(MNM_GLOBAL_REQUEST_ID, idx);
  connected = cb.waitBlocking(timeout);
  MNMSysexListener.removeOnGlobalMessageCallback(&cb);
  return connected;
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
