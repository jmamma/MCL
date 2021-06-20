/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MD_H__
#define MD_H__

#include "WProgram.h"

#include "Elektron.h"

#include "MDMessages.h"
#include "MDParams.h"
#include "MDSysex.h"

/** Standard elektron sysex header for communicating with the machinedrum. **/
extern uint8_t machinedrum_sysex_hdr[5];

class MDMidiEvents : public MidiCallback {
public:
  bool kitupdate_state;

  void enable_live_kit_update();
  void disable_live_kit_update();

  uint8_t last_md_param;
  uint16_t mute_mask_track;
  void onControlChangeCallback_Midi(uint8_t *msg);
  void onControlChangeCallback_Midi2(uint8_t *msg);
  void onNoteOnCallback_Midi(uint8_t *msg);
};

/**
 * This is the main class used to communicate with a Machinedrum
 * connected to the Minicommand.
 *
 * It is used to stored the current settings of the MachineDrum (like
 * the current global, kit, pattern), the current kit settings of the
 *MachineDrum.
 *
 * It is also used to communicate with the MD by providing meaningful
 * functions sending out notes and sysex to the MachineDrum.
 *
 * It also incorporates the mechanics to produce notes on the
 * MachineDrum by doing lookups of pitch information.
 **/
class MDClass : public ElektronDevice {

public:
  MDClass();
  MDMidiEvents midi_events;
  /** Stores the kit settings of the machinedrum (usually set by MDTask). **/
  MDKit kit;
  MDPattern pattern;

  uint16_t mute_mask;
  // uint32_t swing_last;

  /**
   * Stores the global settings of the machinedrum (usually set by MDTask).
   *
   * This is used by most methods of the MDClass because they look up
   * the channel settings and the trigger settings of the MachineDrum.
   **/
  MDGlobalLight global;

  virtual bool probe();
  virtual void setup();
  virtual void init_grid_devices();
  virtual uint8_t* icon();

  // TODO not necessary if we have FW_CAP_READ_LIVE_KIT
  virtual bool canReadWorkspaceKit() { return true; }
  virtual bool canReadKit() { return true; }

  virtual ElektronSysexObject *getKit() { return &kit; }
  virtual ElektronSysexObject *getPattern() { return &pattern; }
  virtual ElektronSysexObject *getGlobal() { return nullptr; }
  virtual ElektronSysexListenerClass *getSysexListener() {
    return &MDSysexListener;
  }

  //Global config
  void setBaseChannel(uint8_t channel);
  void setLocalOn(bool localOn);
  void setProgramChange(uint8_t val);
  void setExternalSync();
  //---
  virtual void updateKitParams();
  virtual uint16_t sendKitParams(uint8_t *mask);
  virtual const char* getMachineName(uint8_t machine);

  /**
   * When given the channel and the cc of an incoming CC messages,
   * this returns the track and the parameter controller by the
   * message. This uses the channel settings of the Global settings.
   *
   * track is set from 0 to 15, or to 255 if the message could not be parsed.
   *
   * param is set from 0 to 23.
   * If the controlled parameter is a mute, the param value is 32.
   * If the controlled parameter is a channel LEVEL, the param value is 33.
   *
   *
   * If the messages could not be interpreted, track is set to 255.
   **/
  void parseCC(uint8_t channel, uint8_t cc, uint8_t *track, uint8_t *param);

  /**
   * Trigger the track with the given velocity, using the channel
   * settings and the trigger settings out of the global settings.
   *
   * track goes from 0 to 15, velocity from 0 to 127.
   **/
  void triggerTrack(uint8_t track, uint8_t velocity, MidiUartParent *uart_ = nullptr);
  /**
   * Set the parameter param (0 to 23, or 32 for mute, and 33 for
   * LEVEL) of the given track (from 0 to 15) to value.
   *
   * Uses the channel settings out of the global settings.
   **/

  ALWAYS_INLINE() void setTrackParam_inline(uint8_t track, uint8_t param, uint8_t value, MidiUartParent *uart_ = nullptr);
  void setTrackParam(uint8_t track, uint8_t param, uint8_t value, MidiUartParent *uart_ = nullptr);

  void setSampleName(uint8_t slot, char *name);

  /** Set the value of the FX parameter to the given value.
   * Type should be one of:
   *
   * - MD_SET_RHYTHM_ECHO_PARAM_ID
   * - MD_SET_GATE_BOX_PARAM_ID
   * - MD_SET_EQ_PARAM_ID
   * - MD_SET_DYNAMIX_PARAM_ID
   **/

  // Send multiple values simultaneously (single sysex message);
  uint8_t sendFXParamsBulk(uint8_t *values, bool send = true);
  uint8_t sendFXParams(uint8_t *values, uint8_t type, bool send = true);

  uint8_t setEchoParams(uint8_t *values, bool send = true);
  uint8_t setReverbParams(uint8_t *values, bool send = true);
  uint8_t setEQParams(uint8_t *values, bool send = true);
  uint8_t setCompressorParams(uint8_t *values, bool send = true);

  uint8_t sendFXParam(uint8_t param, uint8_t value, uint8_t type,
                      bool send = true);
  /** Set the value of an ECHO FX parameter. **/
  uint8_t setEchoParam(uint8_t param, uint8_t value, bool send = true);
  /** Set the value of a REVERB FX parameter. **/
  uint8_t setReverbParam(uint8_t param, uint8_t value, bool send = true);
  /** Set the value of an EQ FX parameter. **/
  uint8_t setEQParam(uint8_t param, uint8_t value, bool send = true);
  /** Set the value of a COMPRESSOR FX parameter. **/
  uint8_t setCompressorParam(uint8_t param, uint8_t value, bool send = true);
  /**
   * Get the actual PITCH value for the MIDI pitch for the given
   * track. If the track is melodic, this will lookup the actual PITCH
   * setting to be set on the machinedrum by using the pitch lookup
   * table.
   *
   * If no appropriate pitch could be found or if the machine is not a
   * melodic machine, this returns 128.
   *
   * This uses the kit information stored in the kit variable.
   **/
  uint8_t trackGetPitch(uint8_t track, uint8_t pitch);
  /**
   * This is the inverse of the trackGetPitch() method, and returns
   * the MIDI pitch for an actual PITCH value received over CC.
   *
   * This returns 128 if no appropriate MIDI pitch could be found or
   * if the machine is not a melodic machine.
   *
   * This uses the kit information stored in the kit variable.
   **/
  uint8_t trackGetCCPitch(uint8_t track, uint8_t cc, int8_t *offset = NULL);

  /**
   * Trigger the given melodic note on the given track. This first
   * sends a CC message to set the actual PITCH parameter of the
   * machine, followed by a note message triggering the given track.
   *
   * If no pitch information could be found or the machine is not
   * melodic, the machine will not be triggered.
   *
   * This uses the kit information stored in the kit variable, as well
   * as the channel settings and the trigger track settings stored in
   * the global variable.
   **/
  void sendNoteOn(uint8_t track, uint8_t pitch, uint8_t velocity);
  void parallelTrig(uint16_t mask, MidiUartParent *uart_ = nullptr);
  void sync_seqtrack(uint8_t length, uint8_t speed, uint8_t step_count, MidiUartParent *uart_ = nullptr);
  /**
   * Slice the track (assuming it's a ROM or RAM-P machine) on the
   * given 32th, assuming that the loaded sample is 2 bars long.
   *
   * The correct flag is used to "correct" the fact that 127 is the
   * end of the sample, thus the middle being not exactly 64, but
   * rather 65.
   **/
  void sliceTrack32(uint8_t track, uint8_t from, uint8_t to,
                    bool correct = true);
  /**
   * Slice the track on 16th boundaries.
   **/
  void sliceTrack16(uint8_t track, uint8_t from, uint8_t to);

  /**
   * Get the tuning information of the given model.
   **/
  const tuning_t *getModelTuning(uint8_t model, bool tonal = false);

  const tuning_t *getKitModelTuning(uint8_t track) {
    return getModelTuning(kit.get_model(track),kit.get_tonal(track));
  }
  /**
   * Lookup the given pitch in the global track trigger settings, and returns
   *the track mapped to the give note. Returns 128 if no track could be found.
   **/
  uint8_t noteToTrack(uint8_t pitch);
  /**
   * Returns true if the machine on the given track (0 to 15) is melodic.
   **/
  bool isMelodicTrack(uint8_t track);

  /**
   * Set the LFO parameter of track to the given value.
   **/
  void setLFOParam(uint8_t track, uint8_t param, uint8_t value);
  /**
   * Set the whole LFO on the given track to the parameters in the lfo
   *structure.
   **/
  void setLFO(uint8_t track, MDLFO *lfo, bool extra = true);

  /**
   * Send a sysex machine to change the model of the machine on the given track.
   **/
  void assignMachine(uint8_t track, uint8_t model, uint8_t init = 255);
  uint8_t assignMachineBulk(uint8_t track, MDMachine *machine, uint8_t send_level, uint8_t mode = 255, bool send = true);
  /**
   * Load the given machine (including parameters) on the given track
   * out of the machine structure.
   **/

  void setMachine(uint8_t track, MDKit *kit);

  void setMachine(uint8_t track, MDMachine *machine);

  /**
   * Load machine, but only send parameters that differ from MD.kit, returns
   *total bytes sent if send == false, then only return the byte count, don't
   *send. if send == true, send parameters to MD, insert machine in kit.
   **/
  uint8_t sendMachine(uint8_t track, MDMachine *machine, bool send_level,
                      bool send = true);

  /**
   * Inserts a machine in to the MDKit object
   **/

  void insertMachineInKit(uint8_t track, MDMachine *machine,
                          bool set_level = true);
  /**
   * Mute/unmute the given track (0 to 15) by sending a CC
   * message. This uses the global channel settings.
   **/
  void muteTrack(uint8_t track, bool mute = true);
  /** Unmute the given track. **/
  void unmuteTrack(uint8_t track) { muteTrack(track, false); }

  /**
   * Send a sysex message to map the given pitch to the given track.
   **/
  void mapMidiNote(uint8_t pitch, uint8_t track);
  /**
   * Send a sysex message to reset the MIDI map to the elektron default
   *settings.
   **/
  void resetMidiMap();

  static const uint8_t OUTPUT_A = 0;
  static const uint8_t OUTPUT_B = 1;
  static const uint8_t OUTPUT_C = 2;
  static const uint8_t OUTPUT_D = 3;
  static const uint8_t OUTPUT_E = 4;
  static const uint8_t OUTPUT_F = 5;
  static const uint8_t OUTPUT_MAIN = 6;

  /**
   * Send a sysex message to route the track (0 to 15) to the given output.
   **/
  uint8_t setTrackRoutings(uint8_t *values, bool send = true);
  uint8_t setTrackRouting(uint8_t track, uint8_t output, bool send = true);

  /**
   * Set the trigger group of srcTrack to trigger trigTrack.
   **/
  void setTrigGroup(uint8_t srcTrack, uint8_t trigTrack);
  /**
   * Set the mute group of srcTrack to mute trigTrack.
   **/
  void setMuteGroup(uint8_t srcTrack, uint8_t muteTrack);

  /**
   * Send a sysex message to load the given global.
   **/
  void setGlobal(uint8_t id);
  /**
   * Send a sysex message to load the given song.
   **/
  void loadSong(uint8_t song);
  /**
   * Send a sysex message to set the sequencer mode (0 = pattern mode, 1 = song
   *mode).
   **/
  void setSequencerMode(uint8_t mode);
  /**
   * Send a sysex message to load the given global.
   **/
  void setLockMode(uint8_t mode);
  /**
   * Copy the name of the given pattern into the string str.
   **/
  void getPatternName(uint8_t pattern, char str[5]);

  /**
   * Check channel settings to see if MD can receive and send CC for params.
   **/
  bool checkParamSettings();
  /**
   * Check triggers settings to see if MD can be triggered over MIDI.
   **/
  bool checkTriggerSettings();
  /**
   * Check that the clock settings of the MD correspond to the Minicommand
   *settings (not working at the moment).
   **/
  bool checkClockSettings();

  void get_mute_state();

  void send_gui_command(uint8_t command, uint8_t value);

  void toggle_kit_menu();
  void toggle_lfo_menu();
  void hold_up_arrow();
  void release_up_arrow();
  void hold_down_arrow();
  void release_down_arrow();
  void hold_record_button();
  void release_record_button();
  void press_play_button();
  void hold_stop_button();
  void release_stop_button();
  void press_extended_button();
  void press_bankgroup_button();
  void toggle_accent_window();
  void toggle_swing_window();
  void toggle_slide_window();
  void hold_trig(uint8_t trig);
  void release_trig(uint8_t trig);
  void hold_bankselect(uint8_t bank);
  void release_bankselect(uint8_t bank);
  void toggle_tempo_window();
  void hold_function_button();
  void release_function_button();
  void hold_left_arrow();
  void release_left_arrow();
  void hold_right_arrow();
  void release_right_arrow();
  void press_yes_button();
  void press_no_button();
  void hold_scale_button();
  void release_scale_button();
  void toggle_scale_window();
  void toggle_mute_window();
  void press_patternsong_button();
  void toggle_song_window();
  void toggle_global_window();
  void copy();
  void clear();
  void paste();
  void toggle_synth_page();
  void track_select(uint8_t track);
  void encoder_button_press(uint8_t encoder);
  void tap_tempo();

  void set_record_off();
  void set_record_on();
  void clear_all_windows();
  void clear_all_windows_quick();

  void copy_pattern();
  void paste_pattern();

  void tap_left_arrow(uint8_t count = 1);
  void tap_right_arrow(uint8_t count = 1);
  void tap_up_arrow(uint8_t count = 1);
  void tap_down_arrow(uint8_t count = 1);
  void enter_global_edit();
  void enter_sample_mgr();
  void rec_sample(uint8_t pos = 255);
  void send_sample(uint8_t pos = 255);
  void preview_sample(uint8_t pos);

  void setSysexRecPos(uint8_t rec_type, uint8_t position);
};

/**
 * The standard always present object representing the MD to which the
 * minicommand is connected.
 **/
extern MDClass MD;
extern const ElektronSysexProtocol md_protocol;

/* @} */

#endif /* MD_H__ */
