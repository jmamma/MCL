/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MDMESSAGES_H__
#define MDMESSAGES_H__

#include "MDPattern.h"
#include "MDParams.h"

extern uint8_t machinedrum_sysex_hdr[5];

/**
 * \addtogroup MD Elektron MachineDrum
 *
 * @{
 *
 * \addtogroup md_sysex MachineDrum Sysex Messages
 *
 * @{
 **/

/**
 * \addtogroup md_sysex_global MachineDrum Global Message
 * @{
 **/

/**
 * This class stores the global settings of the machinedrum, which comprises:
 *
 * - MIDI channel and clock and trigger settings
 * - MachineDrum clock configuration
 * - routing of the individual tracks to the audio outputs
 * - gate, sensitivity and levels of the audio inputs
 **/

class MDGlobalLight {
public:
  MDGlobalLight() { 
    init();
  }

  void init() {
    uint8_t standardDrumMapping[16] = {36, 38, 40, 41, 43, 45, 47, 48,
                                     50, 52, 53, 55, 57, 59, 60, 62}; 
    memcpy(drumMapping, standardDrumMapping, 16);
  }

  uint8_t drumRouting[16];
  int8_t drumMapping[16];
  uint8_t baseChannel;

  uint16_t tempo;
  uint8_t extendedMode;
  bool clockIn;
  bool clockOut;
  bool transportIn;
  bool transportOut;
  bool localOn;

  uint8_t programChange;
  uint8_t trigMode;
};

class MDGlobal: public ElektronSysexObject {
  /**
   * \addtogroup md_sysex_global
   * @{
   **/

public:
  /* DO NOT CHANGE THE ORDER OF DECLARATION OF THESE VARIABLES. */

  /** Original position of the global inside the MD (0 to 7). **/
  uint8_t origPosition;
  /** Stores the audio output for each track. **/
  uint8_t drumRouting[16];
  /** Stores the MIDI pitch that triggers each track. **/
  int8_t drumMapping[16];
  /** Stores the MIDI pitch that triggers each pattern. **/
  uint8_t keyMap[128] = {
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 0,   255, 1,   255, 2,   3,   255, 4,   255,
      5,   255, 6,   7,   255, 8,   255, 9,   10,  255, 11,  255, 12,  255, 13,
      14,  255, 15,  255, 16,  17,  255, 18,  255, 19,  255, 20,  21,  255, 22,
      255, 23,  24,  255, 25,  255, 26,  255, 27,  28,  255, 29,  255, 30,  31,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255};

  /** The MIDI base channel of the MachineDrum. **/
  uint8_t baseChannel;
  uint8_t unused;

  uint16_t tempo;
  uint8_t extendedMode;
  bool clockIn;
  bool clockOut;
  bool transportIn;
  bool transportOut;
  bool localOn;

  uint8_t drumLeft;
  uint8_t drumRight;
  uint8_t gateLeft;
  uint8_t gateRight;
  uint8_t senseLeft;
  uint8_t senseRight;
  uint8_t minLevelLeft;
  uint8_t minLevelRight;
  uint8_t maxLevelLeft;
  uint8_t maxLevelRight;

  uint8_t programChange;
  uint8_t trigMode;

  MDGlobal() : ElektronSysexObject() {};

  virtual uint8_t getPosition() { return origPosition; }
  virtual void setPosition(uint8_t pos) { origPosition = pos; }
  virtual bool fromSysex(MidiClass *midi);
  virtual uint16_t toSysex(ElektronDataToSysexEncoder *encoder);
  /* @} */
};

/* @} */

/**
 * \addtogroup md_sysex_kit MachineDrum Kit Message
 * @{
 **/

/**
 * This class stores the LFO settings for a track, inside the Kit object.
 **/
class MDLFO {
  /**
   * \addtogroup md_sysex_kit
   * @{
   **/

public:
  /* DO NOT CHANGE THE ORDER OF DECLARATION OF THESE PARAMETERS */

  /** The destination track of this LFO. **/
  uint8_t destinationTrack;
  /** The destination parameter of this LFO. **/
  uint8_t destinationParam;
  /** The first shape of this LFO. **/
  uint8_t shape1;
  /** The second shape of this LFO. **/
  uint8_t shape2;
  /** The LFO type. **/
  uint8_t type;
  /** The internal state of the LFO, must not all be 0!. **/
  uint8_t state[31];
  /** The LFO speed. **/
  uint8_t speed;
  /** The LFO depth. **/
  uint8_t depth;
  /** The LFO mix setting. **/
  uint8_t mix;

  void init(uint8_t track) {
    memset(&destinationTrack,0,sizeof(this));
    destinationTrack = track;
    speed = 64;
    uint16_t *lfo_states2 = (uint16_t *) &state[5 + 18];
    lfo_states2[1] = 0x29a; //666
  }
  /* @} */
};

/**
 * This class stores the complete settings for a track inside the Kit object.
 **/
class MDMachine {
  /**
   * \addtogroup md_sysex_kit
   * @{
   **/

public:
  uint8_t params[24];
  uint8_t track;
  uint8_t level;
  uint32_t model;
  MDLFO lfo;
  uint8_t trigGroup;
  uint8_t muteGroup;

  void scale_vol(float scale);
  float normalize_level();
  void init() {
  uint8_t init_params[24] = { 0, 0, 0, 0,
             0, 0, 0, 0,
             0, 0, 64, 64,
             0, 127, 0, 0,
             0, 127, 64, 0,
             0, 64, 0, 0 };
  memcpy(&params,&init_params, sizeof(params));
  level = 127;
  model = GND_MODEL;
  trigGroup = 127;
  muteGroup = 127;
  lfo.init(track);
  }

  uint8_t get_model();
  bool get_tonal();

  uint32_t get_model_raw() {
    return model & 0x200FF; //2^17 + 255
  }
  /* @} */
};

/**
 * This class stores the settings for a complete kit on the
 * machinedrum, including effect and machine settings.
 **/
class MDKit: public ElektronSysexObject {
  /**
   * \addtogroup md_sysex_kit
   * @{
   **/

public:

  uint8_t origPosition;
  char name[17];

  /** The parameters for each track. **/
  uint8_t params[16][24];
  /** Duplicate params not included in the origin MD structure */
  uint8_t params_orig[16][24];
  /** The levels of each track. **/
  uint8_t levels[16];
  /** The selected drum model for each track. **/
  uint32_t models[16];
  /** The LFO settings for each track. **/
  MDLFO lfos[16];
  /** The settings of the reverb effect. **/
  uint8_t reverb[8];
  /** The settings of the delay effect. **/
  uint8_t delay[8];
  /** The settings of the EQ effect. **/
  uint8_t eq[8];
  /** The settings of the compressor effect. **/
  uint8_t dynamics[8];
  /** Duplicate fx params not included in the origin MD structure */
  uint8_t fx_orig[4][9];
  /** The trig group selected for each track (255: OFF). **/
  uint8_t trigGroups[16];
  /** The mute group selected for each track (255: OFF). **/
  uint8_t muteGroups[16];

  MDKit(): ElektronSysexObject() {}

  virtual bool fromSysex(MidiClass *midi);
  virtual uint16_t toSysex(ElektronDataToSysexEncoder *encoder);

  uint16_t toSysex();
  /**
   * Swap two machines.
   **/
  void swapTracks(uint8_t srcTrack, uint8_t dstTrack);

  void init_eq();
  void init_dynamix();

  virtual uint8_t getPosition() { return origPosition; }
  virtual void setPosition(uint8_t pos) { origPosition = pos; }

  uint8_t get_model(uint8_t track);
  bool get_tonal(uint8_t track);
  uint8_t get_fx_param(uint8_t fx, uint8_t param) {
    uint8_t ret = 255;
    switch (fx) {
    case MD_FX_ECHO:
      ret = delay[param];
      break;
    case MD_FX_DYN:
      ret = dynamics[param];
      break;
    case MD_FX_REV:
      ret = reverb[param];
      break;
    case MD_FX_EQ:
      ret = eq[param];
    }
    return ret;
  }
  /* @} */
};

/* @} */

/**
 * \addtogroup md_sysex_song MachineDrum Song Message
 * @{
 **/

/**
 * This class stores a single row in a song.
 **/
class MDRow {
  /**
   * \addtogroup md_sysex_song
   * @{
   **/

public:
  /* DO NOT CHANGE THE ORDER OF DECLARATION OF THESE VARIABLES. */
  uint8_t pattern;
  uint8_t kit;
  uint8_t loopTimes;
  uint8_t jump;
  uint16_t mutes;
  uint16_t tempo;
  uint8_t startPosition;

  /* @} */
};

/**
 * This class stores a song of the MachineDrum.
 **/
class MDSong: public ElektronSysexObject {
  /**
   * \addtogroup md_sysex_song
   * @{
   **/

public:
  uint8_t origPosition;
  char name[17];
  MDRow rows[256];
  uint8_t numRows;

  virtual bool fromSysex(MidiClass *midi);
  virtual uint16_t toSysex(ElektronDataToSysexEncoder *encoder);

  virtual uint8_t getPosition() { return origPosition; }
  virtual void setPosition(uint8_t pos) { origPosition = pos; }

  /* @} */
};

/* @} */

#endif /* MDMESSAGES_H__ */
