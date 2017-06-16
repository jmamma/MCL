/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MERGER_H__
#define MERGER_H__

#include "helpers.h"
#include "Midi.h"

/**
 * \addtogroup Midi
 *
 * @{
 **/

/**
 * \addtogroup midi_tools Midi Tools
 *
 * @{
 **/

/**
 * \addtogroup midi_merger Midi Merger Class
 *
 * @{
 **/

class MergerSysexListener : public MidiSysexListenerClass {
	/**
	 * \addtogroup midi_merger 
	 *
	 * @{
	 **/
	
 public:
  MergerSysexListener() {
    ids[0] = 0xFF; // catchall
    ids[1] = 0;
    ids[2] = 0;
  }

  virtual void end();

	/* @} */
};

class Merger : public MidiCallback {
	/**
	 * \addtogroup midi_merger 
	 *
	 * @{
	 **/
	
 public:
  static const uint8_t MERGE_CC_MASK        = _BV(0);
  static const uint8_t MERGE_NOTE_MASK      = _BV(1);
  static const uint8_t MERGE_SYSEX_MASK     = _BV(2);
  static const uint8_t MERGE_AT_MASK        = _BV(3);
  static const uint8_t MERGE_PRGCHG_MASK    = _BV(4);
  static const uint8_t MERGE_CHANPRESS_MASK = _BV(5);
  static const uint8_t MERGE_PITCH_MASK     = _BV(6);

  uint8_t mask;
  MergerSysexListener mergerSysexListener;
  
  Merger(uint8_t _mask = 0) {
    setMergeMask(_mask);
  }

  void on2ByteCallback(uint8_t *msg);
  void on3ByteCallback(uint8_t *msg);

  void setMergeMask(uint8_t _mask);

	/* @} */
};

/* @} @} @} */

#endif /* MERGER_H__ */
