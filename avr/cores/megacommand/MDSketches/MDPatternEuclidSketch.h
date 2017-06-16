/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MD_PATTERN_EUCLID_SKETCH_H__
#define MD_PATTERN_EUCLID_SKETCH_H__

#include <MD.h>
#include <MDPitchEuclidSketch.h>
#include <MDPatternEuclid.h>

/**
 * \addtogroup MD Elektron MachineDrum
 *
 * @{
 * 
 * \addtogroup md_sketches MachineDrum Sketches
 * 
 * @{
 **/

/**
 * \addtogroup md_sketches_pattern_euclid MachineDrum Pattern Euclid Sketch
 *
 * @{
 **/

class MDPatternEuclidSketch : 
public Sketch, MDCallback {
 public:
  MDPatternEuclid euclid;
  MDPitchEuclidConfigPage1 configPage1;
  MDPitchEuclidConfigPage2 configPage2;
  SwitchPage switchPage;
  MDTrackFlashEncoder trackEncoder;

 MDPatternEuclidSketch() : 
  configPage1(&euclid), configPage2(&euclid), trackEncoder("TRK") {
    configPage2.encoders[0] = &trackEncoder; // HACK HACK
  }

  void setup() {
    configPage1.setShortName("EUC");
    configPage2.setShortName("SCL");

    switchPage.initPages(&configPage1, &configPage2);
    switchPage.parent = this;

    MDTask.addOnPatternChangeCallback(this, (md_callback_ptr_t)&MDPatternEuclidSketch::onPatternChange);
    MDSysexListener.addOnPatternMessageCallback(this,
																								(md_callback_ptr_t)&MDPatternEuclidSketch::onPatternMessage);
  }

  virtual void show() {
    if (currentPage() == NULL)
      setPage(&configPage1);
  }

  virtual void doExtra(bool pressed) {
    if (pressed) {
      euclid.randomizePitches();
      euclid.makeTrack(trackEncoder.getValue());
    }
  }

	void getName(char *n1, char *n2) {
		m_strncpy_p(n1, PSTR("EUC "), 5);
		m_strncpy_p(n2, PSTR("LID "), 5);
	}
	
  virtual bool handleEvent(gui_event_t *event) {
    if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
      pushPage(&switchPage);
      return true;
    } else if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
      popPage(&switchPage);
      return true;
    } 
    if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
      MD.requestPattern(MD.currentPattern);
      return true;
    }
    if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
      euclid.randomizePitches();
      euclid.makeTrack(trackEncoder.getValue());
      return true;
    }

    return false;
  }

  void onPatternChange() {
    MD.requestPattern(MD.currentPattern);
  }

  void onPatternMessage() {
    if (euclid.pattern.fromSysex(MidiSysex.data + 5, MidiSysex.recordLen - 5)) {
      char name[5];
      MD.getPatternName(euclid.pattern.origPosition, name);
      GUI.flash_strings_fill("PATTERN", name);
    } else {
      GUI.flash_strings_fill("SYSEX", "ERROR");
    }
  }

};

/* @} @} @} */

#endif /* MD_PATTERN_EUCLID_SKETCH_H__ */
