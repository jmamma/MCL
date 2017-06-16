/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MD_PATTERN_EUCLID_H__
#define MD_PATTERN_EUCLID_H__

#include <MD.h>
#include "MDPitchEuclid.h"

/**
 * \addtogroup MD Elektron MachineDrum
 *
 * @{
 * 
 * \addtogroup md_pattern_euclid MachineDrum Pattern Euclid
 * 
 * @{
 **/

/**
 * This is a subclass of the MDPitchEuclid class that stores the
 * generated pattern directly as param locks on the MachineDrum.
 **/
class MDPatternEuclid : public MDPitchEuclid {
	/**
	 * \addtogroup md_pattern_euclid
	 * @{
	 **/
	
public:
  MDPattern pattern;

  MDPatternEuclid();

	/**
	 * Generate the pattern and store it on the given track. If the
	 * machine on that track is melodic, the generated line will have
	 * different pitches, else the line will just have triggers for the
	 * percussion sound.
	 **/
  void makeTrack(uint8_t trackNum);

	/* @} */
};

/* @} @} */

#endif /* MD_PATTERN_EUCLID_H__ */
				 
