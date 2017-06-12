#ifndef MNMPATTERN_H__
#define MNMPATTERN_H__

#include <inttypes.h>
#include "MNMDataEncoder.hh"
#include "ElektronPattern.hh"

typedef struct mnm_note_s {
  unsigned note : 7;
  unsigned track : 3;
  unsigned position : 6;
} mnm_note_t;

class MNMPattern : public ElektronPattern {
public:
  uint8_t origPosition;

	uint8_t getPosition() { return origPosition; }
	void    setPosition(uint8_t _pos) { origPosition = _pos; }

	/* SUPER IMPORTANT DO NOT CHANGE THE ORDER OF DECLARATION OF THESE VARIABLES */

  uint64_t ampTrigs[6];
  uint64_t filterTrigs[6];
  uint64_t lfoTrigs[6];
  uint64_t offTrigs[6];
  uint64_t midiNoteOnTrigs[6];
  uint64_t midiNoteOffTrigs[6];
  uint64_t triglessTrigs[6];
  uint64_t chordTrigs[6];
  uint64_t midiTriglessTrigs[6];
  uint64_t slidePatterns[6];
  uint64_t swingPatterns[6];
  uint64_t midiSlidePatterns[6];
  uint64_t midiSwingPatterns[6];

  uint32_t swingAmount;
  uint64_t lockPatterns[6];
  uint8_t noteNBR[6][64];

  uint8_t patternLength;
  uint8_t doubleTempo;
  uint8_t kit;
  int8_t patternTranspose;

  static const uint8_t TRANSPOSE_CHROMATIC = 0;
  static const uint8_t TRANSPOSE_MAJOR = 1;
  static const uint8_t TRANSPOSE_MINOR = 2;
  static const uint8_t TRANSPOSE_FIXED = 3;

  int8_t transpose[6];
  uint8_t scale[6];
  uint8_t key[6];

  int8_t midiTranspose[6];
  uint8_t midiScale[6];
  uint8_t midiKey[6];

  static const uint8_t ARP_PLAY_TRUE = 0;
  static const uint8_t ARP_PLAY_UP   = 1;
  static const uint8_t ARP_PLAY_DOWN = 2;
  static const uint8_t ARP_PLAY_CYCLIC = 3;
  static const uint8_t ARP_PLAY_RND = 4;

  static const uint8_t ARP_MODE_OFF = 0;
  static const uint8_t ARP_MODE_KEY = 1;
  static const uint8_t ARP_MODE_SID  = 2;
  static const uint8_t ARP_MODE_ADD  = 3;

  uint8_t arpPlay[6];
  uint8_t arpMode[6];
  uint8_t arpOctaveRange[6];
  uint8_t arpMultiplier[6];
  uint8_t arpDestination[6];
  uint8_t arpLength[6];
  uint8_t arpPattern[6][16];

	uint8_t midiArpPlay[6];
  uint8_t midiArpMode[6];
  uint8_t midiArpOctaveRange[6];
  uint8_t midiArpMultiplier[6];
  uint8_t midiArpLength[6];
  uint8_t midiArpPattern[6][16];

	uint8_t getKit() { return kit; }
	void    setKit(uint8_t _kit) { kit = _kit; }
	
	uint8_t getLength() { return patternLength; }
	void    setLength(uint8_t _len) { patternLength = _len; }

	uint8_t unused[4];

  uint16_t midiNotesUsed;
  uint8_t chordNotesUsed;

	uint8_t unused2;

  uint8_t locksUsed;
  uint8_t numRows;
  int8_t paramLocks[6][64];
	int8_t getLockIdx(uint8_t track, uint8_t param) {
		return paramLocks[track][param];
	}
	void   setLockIdx(uint8_t track, uint8_t param, int8_t value) {
		paramLocks[track][param] = value;
	}
  
  mnm_note_t midiNotes[400];
  mnm_note_t chordNotes[192];

  MNMPattern() : ElektronPattern() {
		maxSteps = 64;
		maxParams = 64;
		maxTracks = 6;
		maxLocks = 62;
		init();
  }
  
  bool fromSysex(uint8_t *sysex, uint16_t len);
  uint16_t toSysex(uint8_t *sysex, uint16_t len);
	uint16_t toSysex(MNMDataToSysexEncoder &encoder);
	
	bool isTrackEmpty(uint8_t track);
	bool isMidiTrackEmpty(uint8_t track);
	
  void clearPattern();
  void clearTrack(uint8_t track);
	void clearMidiTrack(uint8_t track);

  void clearTrig(uint8_t track, uint8_t trig,
								 bool ampTrig, bool filterTrig = false, bool lfoTrig = false,
								 bool triglessTrig = false, bool chordTrig = false);
	void clearTrig(uint8_t track, uint8_t trig) {
		clearTrig(track, trig, true, true, true, true);
	}
  void setTrig(uint8_t track, uint8_t trig,
							 bool ampTrig, bool filterTrig = false, bool lfoTrig = false,
							 bool triglessTrig = false, bool chordTrig = false);
	
	void setTrig(uint8_t track, uint8_t trig) {
		setTrig(track, trig, true, true, true, true);
	}

  bool isTrigSet(uint8_t track, uint8_t trig,
	       bool ampTrig, bool filterTrig = false, bool lfoTrig = false,
	       bool triglessTrig = false, bool chordTrig = false);

	bool isTrigSet(uint8_t track, uint8_t trig) {
		return isAmpTrigSet(track, trig);
	}

	
  void clearAllTrig(uint8_t track, uint8_t trig) {
    clearTrig(track, trig, true, true, true, true, true);
  }
  void setAllTrig(uint8_t track, uint8_t trig) {
    setTrig(track, trig, true, true, true, true);
  }
  void clearAmpTrig(uint8_t track, uint8_t trig) {
    clearTrig(track, trig, true);
  }
  void setAmpTrig(uint8_t track, uint8_t trig) {
    setTrig(track, trig, true);
  }
	bool isAmpTrigSet(uint8_t track, uint8_t trig) {
		return IS_BIT_SET64(ampTrigs[track], trig);
	}
  void clearFilterTrig(uint8_t track, uint8_t trig) {
    clearTrig(track, trig, false, true);
  }
  void setFilterTrig(uint8_t track, uint8_t trig) {
    setTrig(track, trig, false, true);
  }
  void clearLFOTrig(uint8_t track, uint8_t trig) {
    clearTrig(track, trig, false, false, true);
  }
  void setLFOTrig(uint8_t track, uint8_t trig) {
    setTrig(track, trig, false, false, true);
  }
  void clearTriglessTrig(uint8_t track, uint8_t trig) {
    clearTrig(track, trig, false, false, false, true);
  }
  void setTriglessTrig(uint8_t track, uint8_t trig) {
    setTrig(track, trig, false, false, false, true);
  }
  void clearChordTrig(uint8_t track, uint8_t trig) {
    clearTrig(track, trig, false, false, false, false, true);
  }
  void setChordTrig(uint8_t track, uint8_t trig) {
    setTrig(track, trig, false, false, false, false, true);
  }

  int8_t getNextEmptyLock();
  void recalculateLockPatterns();

  void setNote(uint8_t track, uint8_t step, uint8_t note);
  void setChordNote(uint8_t track, uint8_t step, uint8_t note);
  void clearChordNote(uint8_t track, uint8_t step, uint8_t note);

#ifdef HOST_MIDIDUINO
  void print();
#endif
};



#endif /* MNMPATTERN_H__ */
