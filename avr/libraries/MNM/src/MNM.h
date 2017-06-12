#ifndef MNM_H__
#define MNM_H__

#include <inttypes.h>

#include "WProgram.h"
#include "MNMMessages.hh"
#include "MNMPattern.hh"
#include "MNMParams.hh"

#include "MNMEncoders.h"

class MNMClass {
 public:
  MNMClass();

  uint8_t currentTrack;
  
  int currentGlobal;
  bool loadedGlobal;
  MNMGlobal global;
  
  int currentKit;
  bool loadedKit;
  MNMKit kit;

  int currentPattern;

  void sendSysex(uint8_t *bytes, uint8_t cnt);
  
  void sendMultiTrigNoteOn(uint8_t note, uint8_t velocity);
  void sendMultiTrigNoteOff(uint8_t note);
  void sendMultiMapNoteOn(uint8_t note, uint8_t velocity);
  void sendMultiMapNoteOff(uint8_t note);
  void sendAutoNoteOn(uint8_t note, uint8_t velocity);
  void sendAutoNoteOff(uint8_t note);
  void sendNoteOn(uint8_t note, uint8_t velocity) {
    sendNoteOn(currentTrack, note, velocity);
  }
  void sendNoteOn(uint8_t track, uint8_t note, uint8_t velocity);
  void sendNoteOff(uint8_t note) {
    sendNoteOff(currentTrack, note);
  }
  void sendNoteOff(uint8_t track, uint8_t note);

  void setParam(uint8_t param, uint8_t value) {
    setParam(currentTrack, param, value);
  }
  void setParam(uint8_t track, uint8_t param, uint8_t value);
  void setAutoParam(uint8_t param, uint8_t value);
  void setAutoLevel(uint8_t level);

  void setMultiEnvParam(uint8_t param, uint8_t value);
  void setMidiParam(uint8_t param, uint8_t value) {
    setMidiParam(currentTrack, param, value);
  }
  void setMidiParam(uint8_t track, uint8_t param, uint8_t value);
  void setTrackPitch(uint8_t pitch) {
    setTrackPitch(currentTrack, pitch);
  }
  void setTrackPitch(uint8_t track, uint8_t pitch);

  void setTrackLevel(uint8_t level) {
    setTrackLevel(currentTrack, level);
  }
  void setTrackLevel(uint8_t track, uint8_t level);

  void triggerTrack(bool amp = false, bool lfo = false, bool filter = false) {
    triggerTrack(currentTrack, amp, lfo, filter);
  }
  void triggerTrackAmp() {
    triggerTrackAmp(currentTrack);
  }
  void triggerTrackAmp(uint8_t track) {
    triggerTrack(track, true, false, false);
  }
  void triggerTrackLFO() {
    triggerTrackLFO(currentTrack);
  }
  void triggerTrackLFO(uint8_t track) {
    triggerTrack(track, false, true, false);
  }
  void triggerTrackFilter() {
    triggerTrackFilter(currentTrack);
  }
  void triggerTrackFilter(uint8_t track) {
    triggerTrack(track, false, false, true);
  }
  void triggerTrack(uint8_t track, bool amp = false, bool lfo = false, bool filter = false);
  
  bool parseCC(uint8_t channel, uint8_t cc, uint8_t *track, uint8_t *param);

  void setStatus(uint8_t id, uint8_t value);

  void loadGlobal(uint8_t id);
  void loadKit(uint8_t id);
  void loadPattern(uint8_t id);
  void loadSong(uint8_t id);

  void setSequencerMode(bool songMode);
  void setAudioMode(bool polyMode);
  void setSequencerModeMode(bool midiMode);
  void setAudioTrack(uint8_t track);
  void setMidiTrack(uint8_t track);

  void setCurrentKitName(char *name);
  void saveCurrentKit(uint8_t id);

  void sendRequest(uint8_t type, uint8_t param);
  void requestKit(uint8_t kit);
  void requestPattern(uint8_t pattern);
  void requestSong(uint8_t song);
  void requestGlobal(uint8_t global);

  void assignMachine(uint8_t model, bool initAll = false, bool initSynth = false) {
    assignMachine(currentTrack, model, initAll, initSynth);
  }
  void assignMachine(uint8_t track, uint8_t model, bool initAll = false, bool initSynth = false);
  void setMachine(uint8_t idx) {
    setMachine(currentTrack, idx);
  }
  void setMachine(uint8_t track, uint8_t idx);

  void setMute(bool mute) {
    setMute(currentTrack, mute);
  }
  void setMute(uint8_t track, bool mute);
  void muteTrack() {
    muteTrack(currentTrack);
  }
  void muteTrack(uint8_t track) {
    setMute(track, true);
  }
  void unmuteTrack() {
    unmuteTrack(currentTrack);
  }
  void unmuteTrack(uint8_t track) {
    setMute(track, false);
  }
  void setAutoMute(bool mute);
  void muteAutoTrack() {
    setAutoMute(true);
  }
  void unmuteAutoTrack() {
    setAutoMute(false);
  }
  
  PGM_P getMachineName(uint8_t machine);
  PGM_P getModelParamName(uint8_t model, uint8_t param);
  void getPatternName(uint8_t pattern, char str[5]);

  void revertToCurrentKit(bool reloadKit = true);
  void revertToCurrentTrack(bool reloadTrack = true) {
    if (!reloadTrack) {
      if (currentTrack != 255) {
	revertToTrack(currentTrack, false);
      }
    } else {
      uint8_t track = getCurrentTrack(500);
      if (track != 255) {
	revertToTrack(track, false);
      }
    }
  }
  void revertToTrack(uint8_t track, bool reloadKit = false);

  uint8_t getBlockingStatus(uint8_t type, uint16_t timeout = 1000);
  uint8_t getCurrentTrack(uint16_t timeout = 1000);
  uint8_t getCurrentKit(uint16_t timeout = 1000);
};

extern MNMClass MNM;

#include "MNMSysex.hh"

#include "MNMTask.hh"

#endif /* MNM_H__ */
