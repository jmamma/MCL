/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MD_H__
#define MD_H__

#include "WProgram.h"

#include "Elektron.hh"


/**
 * \addtogroup MD Elektron MachineDrum
 *
 * @{
 * 
 * \defgroup md_md Elektron Machinedrum Class
 * \defgroup md_callbacks Elektron MachineDrum Callbacks
 *
 **/

/**
 * \addtogroup md_callbacks
 *
 * @{
 * MD Callback class, inherit from this class if you want to use callbacks on MD events.
 **/
class MDCallback {
};

/**
 * Standard method prototype for argument-less MD callbacks.
 **/
typedef void(MDCallback::*md_callback_ptr_t)();
/**
 * Standard method prototype for MD status callbacks, called with the
 * status type and the status parameter. This is used to get the
 * current kit, current pattern, current global, etc...
 **/
typedef void(MDCallback::*md_status_callback_ptr_t)(uint8_t type, uint8_t param);

/**
 * Helper class storing the status and type of a Machinedrum
 * request. This is used to have a blocking call waiting for an answer
 * from the MachineDrum.
 **/
class MDBlockCurrentStatusCallback : public MDCallback {
	/**
	 * \addtogroup md_callbacks
	 * @{
	 **/
	
public:
  uint8_t type;
  uint8_t value;
  bool received;

  MDBlockCurrentStatusCallback(uint8_t _type = 0) {
    type = _type;
    received = false;
    value = 255;
  }

	void onStatusResponseCallback(uint8_t _type, uint8_t param) {
	  
	 // GUI.printf_fill("eHHHH C%h N%h ",value, param);
	  if (type == _type) {
      value = param;
      received = true;
    }
  }

	void onSysexReceived() {
		received = true;
	}

	/* @} */
};


/* @} */

#include "MDSysex.hh"
#include "MDParams.hh"
#include "MDMessages.hh"
#include "MDEncoders.h"

/**
 * \addtogroup md_md
 *
 * @{
 */

/** Standard elektron sysex header for communicating with the machinedrum. **/
extern uint8_t machinedrum_sysex_hdr[5];

/** This structure stores the tuning information of a melodic machine on the machinedrum. **/
typedef struct tuning_s {
	/**
	 * \addtogroup md_md
	 *
	 * @{
	 **/
	
	/** Model of the melodic machine. **/
  uint8_t model;
	/** Base pitch of the melodic machine. **/
  uint8_t base;
	/** Length of the tuning array storing the pitch values for each pitch. **/
  uint8_t len;
  uint8_t offset;
	/** Pointer to an array for pitch values for individual midi notes. **/
  const uint8_t *tuning;

	/* @} */
} tuning_t;

/**
 * This is the main class used to communicate with a Machinedrum
 * connected to the Minicommand.
 *
 * It is used to stored the current settings of the MachineDrum (like
 * the current global, kit, pattern), the current kit settings of the MachineDrum.
 *
 * It is also used to communicate with the MD by providing meaningful
 * functions sending out notes and sysex to the MachineDrum.
 *
 * It also incorporates the mechanics to produce notes on the
 * MachineDrum by doing lookups of pitch information.
 **/
class MDClass {
	/**
	 * \addtogroup md_md
	 *
	 * @{
	 */
	
 public:
  MDClass();

	/** Stores the current global of the MD, usually set by the MDTask. **/
  int currentGlobal;
	/** Stores the current kit of the MD, usually set by the MDTask. **/
  int currentKit;
	int currentTrack;
	/** Stores the current pattern of the MD, usually set by the MDTask. **/
  int currentPattern;

	/** Set to true if the kit was loaded (usually set by MDTask). **/
  bool loadedKit;
	/** Stores the kit settings of the machinedrum (usually set by MDTask). **/
  MDKit kit;
	/** Set to true if the global was loaded (usually set by MDTask). **/
  bool loadedGlobal;
	/**
	 * Stores the global settings of the machinedrum (usually set by MDTask).
	 *
	 * This is used by most methods of the MDClass because they look up
	 * the channel settings and the trigger settings of the MachineDrum.
	 **/
  MDGlobal global;

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
  void triggerTrack(uint8_t track, uint8_t velocity);
	/**
	 * Set the parameter param (0 to 23, or 32 for mute, and 33 for
	 * LEVEL) of the given track (from 0 to 15) to value.
	 *
	 * Uses the channel settings out of the global settings.
	 **/
  void setTrackParam(uint8_t track, uint8_t param, uint8_t value);

	/** Send the given sysex buffer to the MachineDrum. **/
  void sendSysex(uint8_t *bytes, uint8_t cnt);

	/** Set the value of the FX parameter to the given value.
	 * Type should be one of:
	 *
	 * - MD_SET_RHYTHM_ECHO_PARAM_ID
	 * - MD_SET_GATE_BOX_PARAM_ID
	 * - MD_SET_EQ_PARAM_ID
	 * - MD_SET_DYNAMIX_PARAM_ID
	 **/
  void sendFXParam(uint8_t param, uint8_t value, uint8_t type);
	/** Set the value of an ECHO FX parameter. **/
  void setEchoParam(uint8_t param, uint8_t value);
	/** Set the value of a REVERB FX parameter. **/
  void setReverbParam(uint8_t param, uint8_t value);
	/** Set the value of an EQ FX parameter. **/
  void setEQParam(uint8_t param, uint8_t value);
	/** Set the value of a COMPRESSOR FX parameter. **/
  void setCompressorParam(uint8_t param, uint8_t value);

	/**
	 * Send a sysex request to the MachineDrum. All the request calls
	 * are wrapped in appropriate methods like requestKit,
	 * requestPattern, etc...
	 **/
  void sendRequest(uint8_t type, uint8_t param);

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

	/**
	 * Slice the track (assuming it's a ROM or RAM-P machine) on the
	 * given 32th, assuming that the loaded sample is 2 bars long.
	 *
	 * The correct flag is used to "correct" the fact that 127 is the
	 * end of the sample, thus the middle being not exactly 64, but
	 * rather 65.
	 **/
  void sliceTrack32(uint8_t track, uint8_t from, uint8_t to, bool correct = true);
	/**
	 * Slice the track on 16th boundaries.
	 **/
  void sliceTrack16(uint8_t track, uint8_t from, uint8_t to);

	/**
	 * Get the tuning information of the given model.
	 **/
  const tuning_t* getModelTuning(uint8_t model);
	/**
	 * Lookup the given pitch in the global track trigger settings, and returns the track mapped to the give note.
	 * Returns 128 if no track could be found.
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
	 * Set the whole LFO on the given track to the parameters in the lfo structure.
	 **/
  void setLFO(uint8_t track, MDLFO *lfo);

	/**
	 * Send a sysex machine to change the model of the machine on the given track.
	 **/
  void assignMachine(uint8_t track, uint8_t model, uint8_t init = 0);

	/**
	 * Load the given machine (including parameters) on the given track
	 * out of the machine structure.
	 **/
  void setMachine(uint8_t track, MDMachine *machine);

	/**
	 * Mute/unmute the given track (0 to 15) by sending a CC
	 * message. This uses the global channel settings.
	 **/
  void muteTrack(uint8_t track, bool mute = true);
	/** Unmute the given track. **/
  void unmuteTrack(uint8_t track) {
    muteTrack(track, false);
  }

	/**
	 * Send a sysex message to map the given pitch to the given track.
	 **/
  void mapMidiNote(uint8_t pitch, uint8_t track);
	/**
	 * Send a sysex message to reset the MIDI map to the elektron default settings.
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
  void setTrackRouting(uint8_t track, uint8_t output);
	/**
	 * Set the machinedrum tempo.
	 **/
  void setTempo(uint16_t tempo);

	/**
	 * Set the trigger group of srcTrack to trigger trigTrack.
	 **/
  void setTrigGroup(uint8_t srcTrack, uint8_t trigTrack);
	/**
	 * Set the mute group of srcTrack to mute trigTrack.
	 **/
  void setMuteGroup(uint8_t srcTrack, uint8_t muteTrack);

	/**
	 * Send a sysex message to set the type id to the given value. This
	 * is used by more specific methods and there should be no need to
	 * use this method directly.
	 **/
  void setStatus(uint8_t id, uint8_t value);
	/**
	 * Send a sysex message to load the given global.
	 **/
  void loadGlobal(uint8_t id);
	/**
	 * Send a sysex message to load the given kit.
	 **/
  void loadKit(uint8_t kit);
	/**
	 * Send a sysex message to load the given pattern.
	 **/
  void loadPattern(uint8_t pattern);
	/**
	 * Send a sysex message to load the given song.
	 **/
  void loadSong(uint8_t song);
	/**
	 * Send a sysex message to set the sequencer mode (0 = pattern mode, 1 = song mode).
	 **/
  void setSequencerMode(uint8_t mode);
	/**
	 * Send a sysex message to load the given global.
	 **/
  void setLockMode(uint8_t mode);

	/**
	 * Save the current kit at the given position.
	 **/
  void saveCurrentKit(uint8_t pos);

	/**
	 * Return a pointer to a program-space string representing the name of the given machine.
	 **/
  PGM_P getMachineName(uint8_t machine);
	/**
	 * Copy the name of the given pattern into the string str.
	 **/
  void getPatternName(uint8_t pattern, char str[5]);

	/**
	 * Request a kit from the machinedrum, which will answer by sending a long sysex message.
	 * Register a callback with the MDSysexListener to act on that message.
	 **/
  void requestKit(uint8_t kit);
	/**
	 * Request a pattern from the machinedrum, which will answer by sending a long sysex message.
	 * Register a callback with the MDSysexListener to act on that message.
	 **/
  void requestPattern(uint8_t pattern);
	/**
	 * Request a song from the machinedrum, which will answer by sending a long sysex message.
	 * Register a callback with the MDSysexListener to act on that message.
	 **/
  void requestSong(uint8_t song);
	/**
	 * Request a global from the machinedrum, which will answer by sending a long sysex message.
	 * Register a callback with the MDSysexListener to act on that message.
	 **/
  void requestGlobal(uint8_t global);

  /**
	 * Check channel settings to see if MD can receive and send CC for params.
	 **/
  bool checkParamSettings();
  /**
	 * Check triggers settings to see if MD can be triggered over MIDI.
	 **/
  bool checkTriggerSettings();
  /**
	 * Check that the clock settings of the MD correspond to the Minicommand settings (not working at the moment).
	 **/
  bool checkClockSettings();

  /* requests */
  /**
	 * Wait for a blocking answer to a status request. Timeout is in clock ticks.
	 **/
	bool waitBlocking(MDBlockCurrentStatusCallback *cb, uint16_t timeout = 3000);
  /**
	 * Get the status answer from the machinedrum, blocking until either
	 * a message is received or the timeout has run out.
	 *
	 * This method normally doesn't have to be used, because the
	 * standard requests (get kit, get pattern, etc...) are covered by
	 * their own methods.
	 **/
  uint8_t getBlockingStatus(uint8_t type, uint16_t timeout = 3000);
	/**
	 * Get the given kit of the machinedrum, blocking for an answer.
	 * The sysex message will be stored in the Sysex receive buffer.
	 **/
  bool getBlockingKit(uint8_t kit, uint16_t timeout = 3000);
	/**
	 * Get the given pattern of the machinedrum, blocking for an answer.
	 * The sysex message will be stored in the Sysex receive buffer.
	 **/
  bool getBlockingPattern(uint8_t pattern, uint16_t timeout = 3000);
	/**
	 * Get the given song of the machinedrum, blocking for an answer.
	 * The sysex message will be stored in the Sysex receive buffer.
	 **/
  bool getBlockingSong(uint8_t song, uint16_t timeout = 3000);
	/**
	 * Get the given global of the machinedrum, blocking for an answer.
	 * The sysex message will be stored in the Sysex receive buffer.
	 **/
  bool getBlockingGlobal(uint8_t global, uint16_t timeout = 3000);
	/**
	 * Get the current kit of the machinedrum, blocking for an answer.
	 **/
  uint8_t getCurrentKit(uint16_t timeout);
	/**
	 * Get the current pattern of the machinedrum, blocking for an answer.
	 **/
  uint8_t getCurrentPattern(uint16_t timeout);

	/* @} */
	uint8_t getCurrentTrack(uint16_t timeout);

};

/**
 * The standard always present object representing the MD to which the
 * minicommand is connected.
 **/
extern MDClass MD;

/* @} */

#include "MDTask.hh"

#endif /* MD_H__ */
