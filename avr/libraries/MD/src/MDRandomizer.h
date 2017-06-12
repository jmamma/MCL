/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MDRANDOMIZER_H__
#define MDRANDOMIZER_H__

#include <inttypes.h>
#include "Stack.h"
#include "Midi.h"

/**
 * \addtogroup MD Elektron MachineDrum
 *
 * @{
 * 
 * \addtogroup md_randomizer MachineDrum machine randomizer
 * 
 * @{
 **/


/**
 * Randomize a track, according to a selected parameter mask, and a variable amount.
 **/
class MDRandomizerClass : public MidiCallback {
	/**
	 * \addtogroup md_randomizer
	 * @{
	 **/
	
 public:
  static const uint32_t paramSelectMask[13];
	static const uint8_t FILTER_MASK = 0;
	static const uint8_t AMD_MASK    = 1;
	static const uint8_t EQ_MASK     = 2;
	static const uint8_t EFFECT_MASK = 3;
	static const uint8_t LOWSYN_MASK = 4;
	static const uint8_t UPSYN_MASK  = 5;
	static const uint8_t SYN_MASK    = 6;
	static const uint8_t LFO_MASK    = 7;
	static const uint8_t SENDS_MASK  = 8;
	static const uint8_t DIST_MASK   = 9;
	static const uint8_t FXLOW_MASK  = 10;
	static const uint8_t FXSYN_MASK  = 11;
	static const uint8_t ALL_MASK    = 12;
  static const char *selectNames[13];

  MDRandomizerClass() {
    track = 0;
  }
  
  Stack<uint8_t [24], 8> undoStack;
  uint8_t origParams[24];
  uint8_t track;

  void setTrack(uint8_t _track);

  void setup();
  
  void randomize(int amount, uint8_t mask, uint8_t *params = NULL);
  bool undo();
  void revert();
  void morphOrig(uint8_t value);
  void morphLast(uint8_t value);
  
  void onKitChanged();
  void onCCCallback(uint8_t *msg);

	/* @} */
};

/* @} @} */

#endif /* MDRANDOMIZER_H__ */
