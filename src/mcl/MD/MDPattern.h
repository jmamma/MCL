/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MDPATTERN_H__
#define MDPATTERN_H__

#include "ElektronPattern.h"
#include "MDParams.h"
#include <inttypes.h>

//#define MDPATTERN_TOSYSEX_ENABLE

#if !defined(__AVR__)
// Number of lock slots per track in the SPS-X (v0x40) pattern wire format.
// Must match the host SPSXSeqDefines NUM_LOCKS so ext_locks_params round-trips
// without truncation.
#define MD_PATTERN_LOCK_SLOTS 34
#endif

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
 * \addtogroup md_pattern_global MachineDrum Pattern Message
 * @{
 **/

class MDPattern : public ElektronPattern {
  /**
   * \addtogroup md_pattern_global
   * @{
   **/
public:
  uint8_t origPosition;

  /* SUPER IMPORTANT DO NOT CHANGE THE ORDER OF DECLARATION OF THESE VARIABLES
   */

  /**
   * Stores the trigger patterns for each track as a 64-bit bit mask (bit set:
   *trigger). Use the 64-bit version of the bit-accessing macros to manipulate
   *it.
   **/
  uint64_t trigPatterns[16];
  /**
   * Stores the lockPattern for each track as a bit mask (bit set:
   *parameter is locked). uint64_t on rp2040 to support params 24-33.
   **/
#if !defined(__AVR__)
  uint64_t lockPatterns[16];
#else
  uint32_t lockPatterns[16];
#endif

  /** Stores the accent pattern as a 64-bit bitmask. **/
  uint64_t accentPattern;
  /** Stores the accent pattern as a 64-bit bitmask. **/
  uint64_t slidePattern;
  /** Stores the accent pattern as a 64-bit bitmask. **/
  uint64_t swingPattern;
  /** Stores the swing amount as a 32-bit value. **/
  uint32_t swingAmount;

  uint8_t accentAmount;
  uint8_t patternLength;
  uint8_t doubleTempo;
  uint8_t scale;

  uint8_t kit;
  uint8_t numLockedRows; // unused

  uint32_t accentEditAll;
  uint32_t slideEditAll;
  uint32_t swingEditAll;
  uint64_t accentPatterns[16];
  uint64_t slidePatterns[16];
  uint64_t swingPatterns[16];

  uint8_t numRows;
#if !defined(__AVR__)
  int8_t paramLocks[16][MD_PARAMS_PER_TRACK];
#else
  int8_t paramLocks[16][24];
#endif

  bool isExtraPattern;

#if !defined(__AVR__)
  /** SPS-X extension fields (rp2040 only) **/
  uint8_t version;
  int8_t ext_microtiming[16][64];
  uint8_t ext_step_flags[16][64];
  uint8_t ext_locks_params[16][MD_PATTERN_LOCK_SLOTS];
  uint8_t ext_track_lengths[16];
  uint8_t ext_track_speeds[16];
  uint16_t patternTempo;
  uint8_t chain_change;
#endif

  /** ElektronPattern implementation */

  virtual uint8_t getPosition() { return origPosition; }
  virtual void setPosition(uint8_t _pos) { origPosition = _pos; }

  virtual uint8_t getLength() { return patternLength; }
  virtual void setLength(uint8_t _len) { patternLength = _len; }

  virtual uint8_t getKit() { return kit; }
  virtual void setKit(uint8_t _kit) { kit = _kit; }

  virtual bool isEmpty() {
    uint8_t count = 15;
    do {
      if (trigPatterns[count]) {
        return false;
      }
      count--;
    } while (count);
    return true;
  }
  virtual bool isTrackEmpty(uint8_t track);
  virtual bool isTrigSet(uint8_t track, uint8_t trig) {
    return IS_BIT_SET64(trigPatterns[track], trig);
  }

  virtual void clearPattern();
  /*
  virtual void clearTrack(uint8_t track);

  virtual void setTrig(uint8_t track, uint8_t trig) {
    SET_BIT64(trigPatterns[track], trig);
  }
  virtual void setNote(uint8_t track, uint8_t step, uint8_t pitch);
  virtual void clearTrig(uint8_t track, uint8_t trig);

  virtual int8_t getLockIdx(uint8_t track, uint8_t param) {
    return paramLocks[track][param];
  }
  virtual void setLockIdx(uint8_t track, uint8_t param, int8_t value) {
    paramLocks[track][param] = value;
  }
  */
  virtual void recalculateLockPatterns();
  /** ElektronSysexObject implementation */
  virtual bool fromSysex(MidiClass *midi);
  virtual uint16_t toSysex(ElektronDataToSysexEncoder *encoder);

  MDPattern(bool _init = true) : ElektronPattern(_init) {
    maxSteps = 64;
#if !defined(__AVR__)
    maxParams = MD_PARAMS_PER_TRACK;
#else
    maxParams = 24;
#endif
    maxTracks = 16;
    maxLocks = 64;

    isExtraPattern = false;
  }

  /* XXX TODO extra pattern 64 */

  uint16_t toSysex();

  void clear_step_locks(uint8_t track, uint8_t step);

  /**
   * Swap two tracks of the patterns by copying hits, param locks and
   * other information from one to the other. The kit information of
   * course needs to be swapped separately.
   **/
  void swapTracks(uint8_t srcTrack, uint8_t dstTrack);

  /**
   * Reverse a track in a pattern.
   **/
  void reverseTrack(uint8_t track);

  /**
   * Reverse the whole pattern.
   **/
  void reversePattern();

  /**
   * Block-reverse a track in the pattern. The track is first
   * separated into groups of blocKSize hits, and those blocks are
   * then reversed. For example, reversing the 16 step pattern with
   * block size 4 would result in :
   *
   * 0 1 2 3  4 5 6 7  8 9 a b  c d e f ->
   * c d e f  8 9 a b  4 5 6 7  0 1 2 3
   *
   * The same pattern reversed with a slice length of 3 would result in:
   *
   * 0 1 2  3 4 5  6 7 8  9 0 a  b c d  e f
   * e f  b c d  9 0 a  6 7 8  3 4 5  0 1 2
   *
   **/
  void reverseBlockTrack(uint8_t track, uint8_t blockSize);

  /**
   * Block-reverse the whole pattern.
   **/
  void reverseBlockPattern(uint8_t blockSize);

  /* @} */
};

#endif /* MDPATTERN_H__ */
