#include "platform.h"
#include "ElektronPattern.h"

// The base ElektronPattern storage (lockTracks, lockParams, locks) is sized
// at 64 rows. Subclasses may report a larger maxLocks (MDPattern uses 592 to
// describe the total addressable range including its ext_locks extension),
// but the inherited methods can only safely iterate the base 64 slots.
#define EP_BASE_LOCK_SLOTS 64

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
	ep_lock_idx_t idx = getLockIdx(track, param);
	if (idx != -1) {
		clearLockPattern(idx);
	}
}

void ElektronPattern::clearTrackLocks(uint8_t track) {
	for (uint8_t i = 0; i < maxParams; i++) {
		clearParamLocks(track, i);
	}
}

void ElektronPattern::clearLockPattern(ep_lock_idx_t lock) {
	// Bound by base storage, not maxLocks: subclasses (MDPattern) can declare
	// maxLocks > 64 but the base lockTracks/lockParams/locks arrays are 64-wide.
	if (lock < 0 || lock >= EP_BASE_LOCK_SLOTS)
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
	ep_lock_idx_t idx = getLockIdx(track, param);
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
	ep_lock_idx_t idx = getLockIdx(track, param);
	if (idx == -1)
		return;
	locks[idx][step] = 255;
	if (isLockEmpty((uint8_t)idx)) {
		clearLockPattern(idx);
	}
}

uint8_t ElektronPattern::getLock(uint8_t track, uint8_t step, uint8_t param) {
	ep_lock_idx_t idx = getLockIdx(track, param);
	if (idx == -1)
		return 255;
	return locks[idx][step];
}

ep_lock_idx_t ElektronPattern::getNextEmptyLock() {
	// Search only the base storage (lockTracks/lockParams sized [64]).
	// MDPattern's ext_lockTracks live in a separate array and would need an
	// override here to be allocatable from this API.
	ep_lock_max_t cap = maxLocks < EP_BASE_LOCK_SLOTS ? maxLocks : EP_BASE_LOCK_SLOTS;
	for (ep_lock_max_t i = 0; i < cap; i++) {
		if ((lockTracks[i] == -1) && (lockParams[i] == -1)) {
			return (ep_lock_idx_t)i;
		}
	}
	return -1;
}

void ElektronPattern::cleanupLocks() {
	ep_lock_max_t cap = maxLocks < EP_BASE_LOCK_SLOTS ? maxLocks : EP_BASE_LOCK_SLOTS;
	for (ep_lock_max_t i = 0; i < cap; i++) {
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
