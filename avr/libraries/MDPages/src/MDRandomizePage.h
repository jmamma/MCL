/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MD_RANDOMIZE_PAGE_H__
#define MD_RANDOMIZE_PAGE_H__

#include <GUI.h>
#include <MD.h>
#include <MDRandomizer.h>

/**
 * \addtogroup MD Elektron MachineDrum
 *
 * @{
 * 
 * \addtogroup md_pages MachineDrum Pages
 * 
 * @{
 **/

/**
 * \addtogroup md_randomize_page MachineDrum Randomizer Configuration Page
 *
 * @{
 **/

/**
 * This page is used to control the MachineDrum randomizer.
 **/
class MDRandomizePage : public EncoderPage {
	/**
	 * \addtogroup md_randomize_page
	 *
	 * @{
	 **/
 public: 
	MDTrackFlashEncoder trackEncoder;
	RangeEncoder amtEncoder;
	EnumEncoder selectEncoder;
	MDRandomizerClass *randomizer;
    
 MDRandomizePage(MDRandomizerClass *_randomizer) :
	randomizer(_randomizer),
		trackEncoder("TRK"),
		amtEncoder(0, 128, "AMT"),
		selectEncoder(MDRandomizerClass::selectNames, countof(MDRandomizerClass::selectNames), "SEL") {
		encoders[0] = &trackEncoder;
		encoders[1] = &amtEncoder;
		encoders[2] = &selectEncoder;

		randomizer->setTrack(trackEncoder.getValue());
	}

	virtual void loop() {
		if (trackEncoder.hasChanged()) {
			randomizer->setTrack(trackEncoder.getValue());
		}
	}

	void randomize() {
		randomizer->randomize(amtEncoder.getValue(), selectEncoder.getValue());
	}
    
	bool handleEvent(gui_event_t *event) {
		if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
			randomize();
			return true;
		}
		if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
			GUI.setLine(GUI.LINE1);
			if (randomizer->undo()) {
				GUI.flash_p_string_fill(PSTR("UNDO"));
			} 
			else {
				GUI.flash_p_string_fill(PSTR("UNDO XXX"));
			}
			return true;
		}
			
		return false;
	}

	/* @} */
};

/* @} @} @} */

#endif /* MD_RANDOMIZE_PAGE_H__ */
