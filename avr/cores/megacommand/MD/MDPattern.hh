/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MDPATTERN_H__
#define MDPATTERN_H__

#include <inttypes.h>
#include "ElektronPattern.hh"

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

/**
 * This class stores just a few data of a pattern message, for studio
 * firmwares that need to manipulate all the patterns on the MD.
 **/
class MDPatternShort {
	/**
	 * \addtogroup md_pattern_global 
	 * @{
	 **/
public:
	uint8_t origPosition;
	uint8_t kit;
	uint8_t patternLength;
	
	MDPatternShort() {
	}
	
	/** Read in a pattern message from a sysex buffer. **/
	bool fromSysex(uint8_t *sysex, uint16_t len);
	/* @} */
};

class MDPattern : public ElektronPattern {
	/**
	 * \addtogroup md_pattern_global 
	 * @{
	 **/
public:
	uint8_t origPosition;
	
	uint8_t getPosition() { return origPosition; }
	void setPosition(uint8_t _pos) { origPosition = _pos; }
	
	/* SUPER IMPORTANT DO NOT CHANGE THE ORDER OF DECLARATION OF THESE VARIABLES */
	
	/**
	 * Stores the trigger patterns for each track as a 64-bit bit mask (bit set: trigger).
	 * Use the 64-bit version of the bit-accessing macros to manipulate it.
	 **/
	uint64_t trigPatterns[16];
	/**
	 * Stores the lockPattern for each track as 24-bit bit mask (bit set: parameter is locked)
	 **/
	uint32_t lockPatterns[16];
	
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
	
	uint8_t getKit() { return kit; }
	void    setKit(uint8_t _kit) { kit = _kit; }
	
	uint8_t getLength() { return patternLength; }
	void    setLength(uint8_t _len) { patternLength = _len; }
	
	uint32_t accentEditAll;
	uint32_t slideEditAll;
	uint32_t swingEditAll;
	uint64_t accentPatterns[16];
	uint64_t slidePatterns[16];
	uint64_t swingPatterns[16];
	
	uint8_t numRows;
	int8_t paramLocks[16][24];
	int8_t getLockIdx(uint8_t track, uint8_t param) {
		return paramLocks[track][param];
	}
	void setLockIdx(uint8_t track, uint8_t param, int8_t value) {
		paramLocks[track][param] = value;
	}
	
	bool isExtraPattern;
	
	MDPattern() : ElektronPattern() {
		maxSteps = 64;
		maxParams = 24;
		maxTracks = 16;
		maxLocks = 64;
		
		isExtraPattern = false;
		init();
	}
	
	/* XXX TODO extra pattern 64 */
	
	/** Read in a pattern message from a sysex buffer. **/
	bool fromSysex(uint8_t *sysex, uint16_t len);
	/** Convert the pattern object into a sysex buffer to be sent to the machinedrum. **/
	uint16_t toSysex(uint8_t *sysex, uint16_t len);
	/**
	 * Convert the pattern object by using the specified encoder. This
	 * allows the pattern to be sent directly to uart without rendering
	 * it into a big intermediary buffer.
	 **/
	uint16_t toSysex(ElektronDataToSysexEncoder &encoder);
	
	bool isTrackEmpty(uint8_t track);
	
	void clearPattern();
	void clearTrack(uint8_t track);
	
	void clearTrig(uint8_t track, uint8_t trig);
	inline void setTrig(uint8_t track, uint8_t trig) {
		SET_BIT64(trigPatterns[track], trig);
	}
	inline bool isTrigSet(uint8_t track, uint8_t trig) {
		return IS_BIT_SET64(trigPatterns[track], trig);
	}
	void setNote(uint8_t track, uint8_t step, uint8_t pitch);
	
	virtual void recalculateLockPatterns();
	
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

