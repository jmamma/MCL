/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MDBREAKDOWN_H__
#define MDBREAKDOWN_H__

#include "Midi.h"

/**
 * \addtogroup MD Elektron MachineDrum
 *
 * @{
 * 
 * \addtogroup md_breakdown MD Breakdown Class
 *
 * @{
 *
 * The MD Breakdown Class can control the playback of a resampled loop
 * on the MachineDrum, providing features such a s looping a smaller
 * part of the loop (2 beats, 1 beats, etc...), adding in breakdown
 * slices, and also reversing/cutting up the sample in a randomized
 * fashion similar to the SupaTrigga VST.
 *
 * The breakdown class registers itself to the midi clock, and
 * triggers the sampling track on the machinedrum, so that both
 * triggers, channel and clock settings have to be correctly
 * configured.
 *
 **/

/**
 * The different speeds of the repeat function.
 **/
typedef enum {
  REPEAT_2_BARS = 0,
  REPEAT_1_BAR,
  REPEAT_2_BEATS,
  REPEAT_1_BEAT,
  REPEAT_1_8TH,
  REPEAT_1_16TH,
  REPEAT_SPEED_CNT
} 
repeat_speed_type_t;

/**
 * The different possibilities for when the breakdown slice comes in.
 **/
typedef enum {
  BREAKDOWN_NONE = 0,
  BREAKDOWN_2_BARS,
  BREAKDOWN_4_BARS,
  BREAKDOWN_2_AND_4_BARS,
  BREAKDOWN_CNT
}  breakdown_type_t;

/**
 * The breakdown class itself.
 *
 * This class registers itself to the MidiClock sequencing system, and
 * will trigger the RAM-P1 track (configured by the ramP1Track member
 * variable) regularly.
 **/
class MDBreakdown : public ClockCallback {
	/**
	 * \addtogroup md_breakdown
	 * @{
	 **/
	
 public:
	/** Stores the different names of the speeds to be displayed. **/
  static const char *repeatSpeedNames[REPEAT_SPEED_CNT];
  static const uint8_t repeatSpeedMask[REPEAT_SPEED_CNT];

	/** Set to true to mute the breakdown functionality. **/
  bool muted;
  
  static const char *breakdownNames[BREAKDOWN_CNT];
  
  bool supaTriggaActive, restorePlayback, breakdownActive, storedBreakdownActive;
  uint8_t ramP1Track;

  repeat_speed_type_t repeatSpeed;
  breakdown_type_t breakdown;

  MDBreakdown() {
    muted = false;
    supaTriggaActive = false;
    restorePlayback = false;
    breakdownActive = false;
    storedBreakdownActive = false;
    breakdown = BREAKDOWN_NONE;
    repeatSpeed = REPEAT_2_BARS;
    ramP1Track = 127;
  }

	/**
	 * Dynamic setup of the breakdown functionality. Registers with the
	 * sequencing system.
	 **/
  void setup();

	/**
	 * Activate the breakdown functionality.
	 **/	 
  void startBreakdown();
	/**
	 * Deactivate the breakdown functionality.
	 **/
  void stopBreakdown();
  void on16Callback();
  void doBreakdown();

	/**
	 * Activate the supatrigga functionality.
	 **/
  void startSupatrigga();
	/**
	 * Deactivate the supatrigga functionality.
	 **/
  void stopSupatrigga();
  void doSupatrigga();

	/* @} */
};

extern MDBreakdown mdBreakdown;

#endif /* MDBREAKDOWN_H__ */
