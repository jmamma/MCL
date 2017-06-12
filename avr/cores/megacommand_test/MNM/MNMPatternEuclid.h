#ifndef MNM_PATTERN_EUCLID_H__
#define MNM_PATTERN_EUCLID_H__

#include <MNM.h>
#include "PitchEuclid.h"

class MNMPatternEuclid : public PitchEuclid {
 public:
	MNMPattern pattern;

	MNMPatternEuclid();

	void makeTrack(uint8_t trackNum);
};

#endif /* MNM_PATTERN_EUCLID_H__ */
