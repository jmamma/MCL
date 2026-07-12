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
// Maximum number of distinct param-lock rows the host can transmit in a v0x40
// pattern. The base ElektronPattern::locks[64][64] array covers rows 0-63;
// MDPattern adds an extension array (rp2040 only) for rows 64..MAX_LOCK_ROWS-1.
#define MAX_LOCK_ROWS 544
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

#if !defined(__AVR__)
  uint16_t numRows;
  // paramLocks holds row indices (0..MAX_LOCK_ROWS-1). int16_t needed since
  // SPS-X patterns can carry > 127 lock rows.
  int16_t paramLocks[16][SPS_PARAMS_PER_TRACK];
#else
  uint8_t numRows;
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

  /** Extended lock rows (64..MAX_LOCK_ROWS-1). Base class locks[64][64]
   *  covers the first 64 rows; this array extends storage for v0x40 patterns
   *  with deep automation. ext_lockTracks/ext_lockParams are widened so they
   *  can hold the matching track/param mapping for extended rows.
   *  Memory cost: ~30.7 KB on rp2040 only. **/
  int8_t  ext_locks[MAX_LOCK_ROWS - 64][64];
  int16_t ext_lockTracks[MAX_LOCK_ROWS - 64];
  int16_t ext_lockParams[MAX_LOCK_ROWS - 64];
#endif

#if !defined(__AVR__)
  /** Return a pointer to the 64-step int8_t row for the given lock row index.
   *  Rows 0..63 live in the base ElektronPattern::locks; rows 64..MAX_LOCK_ROWS-1
   *  live in MDPattern::ext_locks. */
  inline int8_t *lock_row(uint16_t row) {
    return (row < 64) ? (int8_t*)locks[row] : ext_locks[row - 64];
  }
#else
  // AVR has no ext_locks; only rows 0..63 exist. Provided so the encoder
  // doesn't need #ifdef branches at every access site.
  inline int8_t *lock_row(uint8_t row) { return (int8_t*)locks[row]; }
#endif
#if !defined(__AVR__)
  inline int16_t lock_track(uint16_t row) const {
    return (row < 64) ? (int16_t)lockTracks[row] : ext_lockTracks[row - 64];
  }
  inline int16_t lock_param(uint16_t row) const {
    return (row < 64) ? (int16_t)lockParams[row] : ext_lockParams[row - 64];
  }
  inline void set_lock_track_param(uint16_t row, int16_t track, int16_t param) {
    if (row < 64) {
      lockTracks[row] = (int8_t)track;
      lockParams[row] = (int8_t)param;
    } else {
      ext_lockTracks[row - 64] = track;
      ext_lockParams[row - 64] = param;
    }
  }
#endif

  /** ElektronPattern implementation */

  virtual uint8_t getPosition() { return origPosition; }
  virtual void setPosition(uint8_t _pos) { origPosition = _pos; }

  virtual uint8_t getLength() { return patternLength; }
  virtual void setLength(uint8_t _len) { patternLength = _len; }

  virtual uint8_t getKit() { return kit; }
  virtual void setKit(uint8_t _kit) { kit = _kit; }

  virtual bool isEmpty() {
    for (uint8_t track = 0; track < 16; ++track) {
      if (trigPatterns[track]) {
        return false;
      }
    }
    return true;
  }
  virtual bool isTrackEmpty(uint8_t track);
  virtual bool isTrigSet(uint8_t track, uint8_t trig) {
    if (track >= 16 || trig >= 64) return false;
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
    maxParams = SPS_PARAMS_PER_TRACK;
#else
    maxParams = 24;
#endif
    maxTracks = 16;
    // Total addressable lock rows. Base ElektronPattern storage covers the
    // first 64; MDPattern's ext_locks extension carries rows 64..MAX_LOCK_ROWS-1.
    // The inherited base-class methods (addLock/cleanupLocks/getNextEmptyLock)
    // self-cap at the base 64 slots; ext rows are populated only via
    // set_lock_track_param/lock_row from sysex round-trip.
#if !defined(__AVR__)
    maxLocks = MAX_LOCK_ROWS;
#else
    maxLocks = 64;
#endif

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
