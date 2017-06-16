/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef ELEKTRON_PATTERN_H__
#define ELEKTRON_PATTERN_H__

/**
 * \addtogroup Elektron
 *
 * @{
 *
 * \addtogroup elektron_pattern Elektron Pattern
 *
 * \file
 * Elektron high-level pattern class
 **/

/**
 * Represents an abstracted pattern for an Elektron machine, with a
 * number of tracks, and a max of 64*64 param locks. This is enough
 * for both monomachine and machinedrum and allows to write patches
 * working on either kind of pattern.
 **/
class ElektronPattern {
public:
	uint8_t maxParams;
	uint8_t maxTracks;
	uint8_t maxSteps;
	uint8_t maxLocks;

	uint8_t locks[64][64];
	int8_t lockTracks[64];
	int8_t lockParams[64];

	ElektronPattern() {
		maxParams = 0;
		maxTracks = 0;
		maxSteps = 0;
		maxLocks = 0;
		clearPattern();
	}

	/** Clear the pattern. */
	void init() {
		clearPattern();
	}

	/** Return the position of the pattern in machine memory. **/
	virtual uint8_t getPosition()                 { return 0;	}
	/** Set the position of the pattern in machine memory. **/
	virtual void    setPosition(uint8_t position) { }

	/** Get the number of steps in the pattern. **/
	virtual uint8_t getLength()                   {	return 0;	}
	/** Set the number of steps in the pattern. **/
	virtual void    setLength(uint8_t length)     { }

	/** Get the kit position in machine memory. **/
	virtual uint8_t getKit()                      { return 0; }
	/** Set the kit position in machine memory. **/
	virtual void    setKit(uint8_t kit)           { }

	/** Returns true if the track is empty (machine specific, holder function). **/
	virtual bool isTrackEmpty(uint8_t track)                { return true; }
	/** Returns true if the parameter param on track is empty (machine specific, holder function). **/
	virtual bool isLockEmpty(uint8_t track, uint8_t param)  { return true; }
	/** Returns true if the trigger on track is set (machine specific). **/
	virtual bool isTrigSet(uint8_t track, uint8_t step)     { return false; }
	/** Returns true if the parameter param is locked on track. **/
	bool isParamLocked(uint8_t track, uint8_t param);
	/** Returns true if the parameter param is not locked on track at the given trigs (it could be locked in between). **/
	bool isLockPatternEmpty(int8_t idx, uint64_t trigs);
	/** Returns true if the lock pattern is empty (all values are 255). */
	bool isLockEmpty(uint8_t idx);

	/** Clear pattern (machine specific). **/
	virtual void clearPattern()                         { }
	/** Clear the track (machine specific). **/
	virtual void clearTrack(uint8_t track)              { }
	/** Clear the paramlocks for the parameter param on track. **/
	void clearParamLocks(uint8_t track, uint8_t param);
	/** Clear all the paramlocks on track. **/
	void clearTrackLocks(uint8_t track);
	/** Clear the lock pattern for the given lock. **/
	void clearLockPattern(int8_t lock);

	/** Set the trigger at step on track (machine specific). **/
	virtual void setTrig(uint8_t track, uint8_t step)                { }
	/** Set the note (pitch and trigger) on track at step with the given pitch (machine specific). **/
	virtual void setNote(uint8_t track, uint8_t step, uint8_t pitch) { }
	/** Clear the trigger at step on track (machine specific) **/
	virtual void clearTrig(uint8_t track, uint8_t step)              { }

	/** Add a param lock on track, for parameter param, at the step trig with the given value. **/
	bool addLock(uint8_t track, uint8_t trig, uint8_t param, uint8_t value);
	/** Remove the paramlock for parameter param on track track at the step trig. **/
	void clearLock(uint8_t track, uint8_t trig, uint8_t param);
	/** Get the locked value for parameter param on track track at the step trig. **/
	uint8_t getLock(uint8_t track, uint8_t trig, uint8_t param);

	/** Get the lock index for parameter param on track track (machine specific). **/
	virtual int8_t getLockIdx(uint8_t track, uint8_t param)             { return -1; }
	/** Set the lock index for the parameter param on track track. **/
	virtual void setLockIdx(uint8_t track, uint8_t param, int8_t value) { }
	/** Reorganize the lock patterns to be in the correct order (machine specific). **/
	virtual void recalculateLockPatterns()                              { }

	/** Get the index of the next empty lock, or -1 if no lock is available. **/
	int8_t getNextEmptyLock();
	/** Reorganize the locks and remove empty locked parameters. **/
	void cleanupLocks();

	/** Fill the pattern from sysex (machine specific). **/
	virtual bool fromSysex(uint8_t *sysex, uint16_t len) { return false; }
	
};

#endif /* ELEKTRON_PATTERN_H__ */
