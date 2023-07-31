#ifndef MNM_MESSAGES_H__
#define MNM_MESSAGES_H__

#include <inttypes.h>
#include "MNMDataEncoder.h"
#include "Elektron.h"
#include "MNMParams.h"
/*
class MNMMidiMap {
public:
  uint8_t range;
  uint8_t pattern;
  uint8_t offset;
  uint8_t length;
  int8_t transpose;
  uint8_t timing;
};
*/

class MNMGlobal : public ElektronSysexObject {
public:
  uint8_t origPosition;

	/* XXX Don't change the order of declaration, important for decoding */
	
  uint8_t autotrackChannel;
  uint8_t baseChannel;
  uint8_t channelSpan;
  uint8_t multitrigChannel;
  uint8_t multimapChannel;

  bool clockIn;
  bool clockOut;
  bool ctrlIn;
  bool ctrlOut;

  uint8_t transportIn;
  uint8_t sequencerOut;
  uint8_t arpOut;

  uint8_t transportOut;
  
  uint8_t keyboardOut;
  uint8_t midiClockOut;
  uint8_t pgmChangeOut;

  uint8_t note; /* not used */
  uint8_t gate; /* not used */
  uint8_t sense; /* not used */
  uint8_t minVelocity; /* not used */
  uint8_t maxVelocity; /* not used */

  uint8_t midiMachineChannels[6];
  uint8_t ccDestinations[6][4];

  uint8_t midiSeqLegato[6]; /* always true */
  uint8_t legato[6]; /* not unsed */

	uint8_t mapRange[32];
	uint8_t mapPattern[32];
	uint8_t mapOffset[32];
	uint8_t mapLength[32];
	int8_t mapTranspose[32];
	uint8_t mapTiming[32];

  uint8_t globalRouting;
	bool pgmChangeIn;
	uint8_t unused[6];

  uint32_t baseFreq;

  MNMGlobal() {
  }

  virtual bool fromSysex(MidiClass *midi);
  virtual uint16_t toSysex(ElektronDataToSysexEncoder *encoder);

  virtual uint8_t getPosition() { return origPosition; }
  virtual void setPosition(uint8_t pos) { origPosition = pos; }
};

class MNMTrackModifier {
public:
  static const uint8_t DEST_POS_PITCH_BEND = 0;
  static const uint8_t DEST_NEG_PITCH_BEND = 1;
  static const uint8_t DEST_POS_MOD_WHEEL = 2;
  static const uint8_t DEST_NEG_MOD_WHEEL = 3;
  static const uint8_t DEST_VELOCITY = 4;
  static const uint8_t DEST_KEY_FOLLOW = 5;

  uint8_t destPage[6][2];
  uint8_t destParam[6][2];
  int8_t range[6][2];
  bool mirrorLR;
  bool mirrorUD;
  bool LPKeyTrack;
  bool HPKeyTrack;
  void init() {
    memset(this, 0, sizeof(MNMTrackModifier));
  }
};

class MNMTrig {
public:
  void init(uint8_t track_) {
    portamento = 0;
    track = track_;
    legatoAmp = 0;
    legatoFilter = 0;
    legatoLFO = 0;
  }
  bool portamento;
  uint8_t track;
  bool legatoAmp;
  bool legatoFilter;
  bool legatoLFO;
};

class MNMMachine {
public:
  uint8_t params[72];
  uint8_t level;
  uint8_t track;
  uint8_t type;
  uint8_t model;
  MNMTrig trig;
  MNMTrackModifier modifier;
  void init(uint8_t track_) {
    memset(&params,0, sizeof(params));
    level = 127;
    track = track_;
    model = MNM_GND_GND_MODEL;
    trig.init(track);
    modifier.init();
  }
};

class MNMKit: public ElektronSysexObject {
public:
  uint8_t origPosition;
  char name[12];
	uint8_t levels[6];
	uint8_t parameters[6][72];
	uint8_t models[6];
	uint8_t types[6];
  uint16_t patchBusIn;
	uint8_t mirrorLR;
	uint8_t mirrorUD;
	uint8_t destPages[6][6][2];
	uint8_t destParams[6][6][2];
	int8_t destRanges[6][6][2];
	uint8_t lpKeyTrack;
	uint8_t hpKeyTrack;

	uint8_t trigPortamento;
	uint8_t trigTracks[6];
	uint8_t trigLegatoAmp;
	uint8_t trigLegatoFilter;
	uint8_t trigLegatoLFO;
	
  static const uint8_t MULTIMODE_ALL = 0;
  static const uint8_t MULTIMODE_SPLIT_KEY = 1;
  static const uint8_t MULTIMODE_SEQ_START = 2;
  static const uint8_t MULTIMODE_SEQ_TRANSPOSE = 3;
  uint8_t commonMultimode;
  uint8_t commonTiming;

  uint8_t splitKey;
  uint8_t splitRange;

  MNMKit() {
  }
  
  virtual bool fromSysex(MidiClass *midi);
  virtual uint16_t toSysex(ElektronDataToSysexEncoder *encoder);

  virtual uint8_t getPosition() { return origPosition; }
  virtual void setPosition(uint8_t pos) { origPosition = pos; }
};

class MNMRow {
public:
  uint8_t pattern;
  uint8_t kit;
  uint8_t loop;
  uint8_t jump;
  uint8_t mute;
  uint8_t muteMidi;
  uint8_t startPosition;
  uint8_t endPosition;
  int8_t patternTranspose;
  int8_t trackTranspose[6];
  int8_t midiTranspose[6];
  uint16_t tempo;
};

class MNMSong: public ElektronSysexObject {
public:
  uint8_t origPosition;
  char name[17];
  MNMRow rows[200];

  MNMSong() {
  }
  
  virtual bool fromSysex(MidiClass *midi);
  virtual uint16_t toSysex(ElektronDataToSysexEncoder *encoder);

  virtual uint8_t getPosition() { return origPosition; }
  virtual void setPosition(uint8_t pos) { origPosition = pos; }
};



#endif /* MNM_MESSAGES_H__ */
