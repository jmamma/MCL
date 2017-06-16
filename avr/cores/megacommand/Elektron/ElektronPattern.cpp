#include "WProgram.h"
#include "ElektronPattern.hh"

bool ElektronPattern::isParamLocked(uint8_t track, uint8_t param) {
	return getLockIdx(track, param) != -1;
}

bool ElektronPattern::isLockPatternEmpty(int8_t idx, uint64_t trigs) {
	for (uint8_t i = 0; i < maxSteps; i++) {
		if ((locks[idx][i] != 255) || !IS_BIT_SET64(trigs, i)) {
			return false;
		}
	}
	return true;
}

bool ElektronPattern::isLockEmpty(uint8_t idx) {
	for (uint8_t i = 0; i < maxSteps; i++) {
		if (locks[idx][i] != 255)
			return false;
	}
	return true;
}

void ElektronPattern::clearParamLocks(uint8_t track, uint8_t param) {
	int8_t idx = getLockIdx(track, param);
	if (idx != -1) {
		clearLockPattern(idx);
	}
}

void ElektronPattern::clearTrackLocks(uint8_t track) {
	for (uint8_t i = 0; i < maxParams; i++) {
		clearParamLocks(track, i);
	}
}

void ElektronPattern::clearLockPattern(int8_t lock) {
	if (lock >= maxLocks)
		return;

	for (uint8_t i = 0; i < maxSteps; i++) {
		locks[lock][i] = 255;
	}
	if ((lockTracks[lock] != -1) && (lockParams[lock] != -1)) {
		setLockIdx(lockTracks[lock], lockParams[lock], -1);
	}
	lockTracks[lock] = -1;
	lockParams[lock] = -1;
}

bool ElektronPattern::addLock(uint8_t track, uint8_t step, uint8_t param, uint8_t value) {
	int8_t idx = getLockIdx(track, param);
	if (idx == -1) {
		idx = getNextEmptyLock();
		if (idx == -1)
        { return false; }
		setLockIdx(track, param, idx);
		lockTracks[idx] = track;
		lockParams[idx] = param;
		for (uint8_t i = 0; i < maxSteps; i++) {
			locks[idx][i] = 255;
		}
	}
	locks[idx][step] = value;
	return true;
}

void ElektronPattern::clearLock(uint8_t track, uint8_t step, uint8_t param) {
	int8_t idx = getLockIdx(track, param);
	if (idx == -1)
		return;
	locks[idx][step] = 255;
	if (isLockEmpty(track, param)) {
		clearLockPattern(idx);
	}
}

uint8_t ElektronPattern::getLock(uint8_t track, uint8_t step, uint8_t param) {
	int8_t idx = getLockIdx(track, param);
	if (idx == -1)
		return 255;
	return locks[idx][step];
}

int8_t ElektronPattern::getNextEmptyLock() {
	for (uint8_t i = 0; i < maxLocks; i++) {
		if ((lockTracks[i] == -1) && (lockParams[i] == -1)) {
			return i;
		}
	}
	return -1;
}

void ElektronPattern::cleanupLocks() {
	for (uint8_t i = 0; i < maxLocks; i++) {
		if (lockTracks[i] != -1) {
			uint8_t lockTrack = lockTracks[i];
			if (isLockEmpty(i)) {
				if (lockParams[i] != -1) {
					setLockIdx(lockTrack, lockParams[i], -1);
				}
				lockTracks[i] = -1;
				lockParams[i] = -1;
			}
		} else {
			lockParams[i] = -1;
		}
	}
}
