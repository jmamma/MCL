#ifndef MNM_H__
#define MNM_H__

#include <inttypes.h>

#include "MNMMessages.h"
#include "MNMParams.h"
#include "MNMPattern.h"
#include "MNMSysex.h"
#include "WProgram.h"

class MNMClass : public ElektronDevice {
public:
  MNMClass();
  MidiUartClass2 *midiuart;
  MNMGlobal global;

  MNMKit kit;
  // MNMPattern pattern;

  virtual bool probe();
  virtual void init_grid_devices();
  virtual uint8_t* icon();

  virtual ElektronSysexObject *getKit() { return &kit; }
  virtual ElektronSysexObject *getPattern() { return nullptr; }
  virtual ElektronSysexObject *getGlobal() { return &global; }
  virtual ElektronSysexListenerClass *getSysexListener() {
    return &MNMSysexListener;
  }

  virtual void updateKitParams();
  virtual uint16_t sendKitParams(uint8_t *mask);
  virtual const char* getMachineName(uint8_t machine);

  virtual bool canReadKit() {
    // TODO fw cap for live kit access
    //return fw_caps & FW_CAP
    return true;
  }

  virtual bool getBlockingPattern(uint8_t pattern, uint16_t timeout) {
    // TODO MNM does not get the pattern but reports success.
    return true;
  }

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
  void sendNoteOff(uint8_t note) { sendNoteOff(currentTrack, note); }
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
  void setTrackPitch(uint8_t pitch) { setTrackPitch(currentTrack, pitch); }
  void setTrackPitch(uint8_t track, uint8_t pitch);

  void setTrackLevel(uint8_t level) { setTrackLevel(currentTrack, level); }
  void setTrackLevel(uint8_t track, uint8_t level);

  void triggerTrack(bool amp = false, bool lfo = false, bool filter = false) {
    triggerTrack(currentTrack, amp, lfo, filter);
  }
  void triggerTrackAmp() { triggerTrackAmp(currentTrack); }
  void triggerTrackAmp(uint8_t track) {
    triggerTrack(track, true, false, false);
  }
  void triggerTrackLFO() { triggerTrackLFO(currentTrack); }
  void triggerTrackLFO(uint8_t track) {
    triggerTrack(track, false, true, false);
  }
  void triggerTrackFilter() { triggerTrackFilter(currentTrack); }
  void triggerTrackFilter(uint8_t track) {
    triggerTrack(track, false, false, true);
  }
  void triggerTrack(uint8_t track, bool amp = false, bool lfo = false,
                    bool filter = false);

  bool parseCC(uint8_t channel, uint8_t cc, uint8_t *track, uint8_t *param);

  void loadSong(uint8_t id);

  void setSequencerMode(bool songMode);
  void setAudioMode(bool polyMode);
  void setSequencerModeMode(bool midiMode);
  void setAudioTrack(uint8_t track);
  void setMidiTrack(uint8_t track);

  void assignMachine(uint8_t model, bool initAll = false,
                     bool initSynth = false) {
    assignMachine(currentTrack, model, initAll, initSynth);
  }
  void assignMachine(uint8_t track, uint8_t model, bool initAll = false,
                     bool initSynth = false);

  void insertMachineInKit(uint8_t track, MNMMachine *machine,
                          bool set_level = true);

  void setMachine(uint8_t idx) { setMachine(currentTrack, idx); }
  void setMachine(uint8_t track, uint8_t idx);

  void setMute(bool mute) { setMute(currentTrack, mute); }
  void setMute(uint8_t track, bool mute);
  void muteTrack() { muteTrack(currentTrack); }
  void muteTrack(uint8_t track) { setMute(track, true); }
  void unmuteTrack() { unmuteTrack(currentTrack); }
  void unmuteTrack(uint8_t track) { setMute(track, false); }
  void setAutoMute(bool mute);
  void muteAutoTrack() { setAutoMute(true); }
  void unmuteAutoTrack() { setAutoMute(false); }

  const char* getModelParamName(uint8_t model, uint8_t param);
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
};

extern MNMClass MNM;
extern const ElektronSysexProtocol mnm_protocol;

#include "MNMSysex.h"

#endif /* MNM_H__ */
