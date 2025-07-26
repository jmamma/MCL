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
  MNMGlobal global;

  MNMKit kit;
  // MNMPattern pattern;

  virtual bool probe();
  virtual void init_grid_devices(uint8_t device_idx);
  virtual uint8_t* icon();
  virtual MCLGIF* gif();
  virtual uint8_t* gif_data();

  virtual bool canReadWorkspaceKit() { return true; }
  virtual bool getWorkSpaceKit() {
    return getBlockingKit(0x80);
  }
  virtual void requestKit(uint8_t kit);
  virtual ElektronSysexObject *getKit() { return &kit; }
  virtual char *getKitName() { return kit.name; }
  virtual ElektronSysexObject *getPattern() { return nullptr; }
  virtual ElektronSysexObject *getGlobal() { return &global; }
  virtual ElektronSysexListenerClass *getSysexListener() {
    return &MNMSysexListener;
  }

  virtual uint8_t get_mute_cc() { return 0x03; }
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

  uint8_t setParam(uint8_t param, uint8_t value) {
    return setParam(currentTrack, param, value);
  }
  uint8_t setParam(uint8_t track, uint8_t param, uint8_t value, bool send = true);
  void setAutoParam(uint8_t param, uint8_t value);
  void setAutoLevel(uint8_t level);

  void setMultiEnvParam(uint8_t param, uint8_t value);
  void setMidiParam(uint8_t param, uint8_t value) {
    setMidiParam(currentTrack, param, value);
  }
  void setMidiParam(uint8_t track, uint8_t param, uint8_t value);
  void setTrackPitch(uint8_t pitch) { setTrackPitch(currentTrack, pitch); }
  void setTrackPitch(uint8_t track, uint8_t pitch);

  uint8_t setTrackLevel(uint8_t level) { return setTrackLevel(currentTrack, level); }
  uint8_t setTrackLevel(uint8_t track, uint8_t level, bool send = true);

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

  uint8_t assignMachine(uint8_t model, bool initAll = false,
                     bool initSynth = false) {
    return assignMachine(currentTrack, model, initAll, initSynth);
  }
  uint8_t assignMachine(uint8_t track, uint8_t model, bool initAll = false,
                     bool initSynth = false, bool send = true);

  void insertMachineInKit(uint8_t track, MNMMachine *machine,
                          bool set_level = true);

  uint8_t setMachine(uint8_t idx) { return setMachine(currentTrack, idx); }
  uint8_t setMachine(uint8_t track, uint8_t idx, bool send = true);

  void muteTrack(uint8_t track, bool mute = true, MidiUartParent *uart_ = nullptr);
  /*
  void muteTrack() { muteTrack(currentTrack); }
  void muteTrack(uint8_t track) { setMute(track, true); }
  void unmuteTrack() { unmuteTrack(currentTrack); }
  void unmuteTrack(uint8_t track) { setMute(track, false); }
  */
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
