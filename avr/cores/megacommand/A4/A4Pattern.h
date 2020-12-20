
#ifndef A4PATTERN_H__
#define A4PATTERN_H__

#include <inttypes.h>
#include "ElektronPattern.h"

class A4PatternShort {
	/**
	 * \addtogroup md_pattern_global 
	 * @{
	 **/
public:
	uint8_t origPosition;
	uint8_t kit;
	uint8_t patternLength;
	
	A4PatternShort() {
	}
	
	/** Read in a pattern message from a sysex buffer. **/
	bool fromSysex(uint8_t *sysex, uint16_t len);
	/* @} */
};

class A4Pattern : public ElektronPattern {
	/**
	 * \addtogroup md_pattern_global 
	 * @{
	 **/
public:
    /*Somebody needs to reverse engineer the 14KB payload someday*/

    uint8_t data[8];

	A4Pattern(bool _init = true) : ElektronPattern(_init) {
		maxSteps = 64;
		maxParams = 24;
		maxTracks = 16;
		maxLocks = 64;
	}
	
	/* XXX TODO extra pattern 64 */
	
	/** Read in a pattern message from a sysex buffer. **/
	bool fromSysex(uint8_t *sysex, uint16_t len);
	/** Convert the pattern object into a sysex buffer to be sent to the machinedrum. **/
};

#endif /* A4PATTERN_H__ */

