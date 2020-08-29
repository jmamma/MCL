/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MDPITCHEUCLIDSKETCH_H__
#define MDPITCHEUCLIDSKETCH_H__

#include <MD.h>
#include <MDPitchEuclid.h>

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
 * \addtogroup md_sketches_pitch_euclid MachineDrum Pitch Euclid Sketch
 *
 * @{
 **/

class MDPitchEuclidConfigPage1 : 
public EncoderPage {
 public:
  RangeEncoder pitchLengthEncoder;
  RangeEncoder pulseEncoder;
  RangeEncoder lengthEncoder;
  RangeEncoder offsetEncoder;
  MDPitchEuclid *euclid;

 MDPitchEuclidConfigPage1(MDPitchEuclid *_euclid) :
  euclid(_euclid), 
		pitchLengthEncoder(1, 32, "PTC", 4),
		pulseEncoder(1, 32, "PLS", 3),
		lengthEncoder(2, 32, "LEN", 8),
		offsetEncoder(0, 32, "OFF", 0) {
    encoders[0] = &pitchLengthEncoder;
    encoders[1] = &pulseEncoder;
    encoders[2] = &lengthEncoder;
    encoders[3] = &offsetEncoder;
  }

  void loop() {
    if (pulseEncoder.hasChanged() || lengthEncoder.hasChanged() || offsetEncoder.hasChanged()) {
      euclid->track.setEuclid(pulseEncoder.getValue(), lengthEncoder.getValue(),
															offsetEncoder.getValue());
    }
    if (pitchLengthEncoder.hasChanged()) {
      euclid->setPitchLength(pitchLengthEncoder.getValue());
    }
  }
};

class MDPitchEuclidConfigPage2 : 
public EncoderPage {
 public:
  MDMelodicTrackFlashEncoder trackEncoder;
  RangeEncoder scaleEncoder;
  RangeEncoder octavesEncoder;
  NotePitchEncoder basePitchEncoder;
  MDPitchEuclid *euclid;

 MDPitchEuclidConfigPage2(MDPitchEuclid *_euclid) :
  euclid(_euclid),
		trackEncoder("TRK", 0),
		scaleEncoder(0, MDPitchEuclid::NUM_SCALES - 1, "SCL", 0),
		basePitchEncoder("BAS"),
		octavesEncoder(0, 4, "OCT")
			{
				encoders[0] = &trackEncoder;
				encoders[3] = &basePitchEncoder;
				encoders[2] = &octavesEncoder;
				encoders[1] = &scaleEncoder;
			}

  void loop() {
    if (scaleEncoder.hasChanged()) {
      euclid->currentScale = MDPitchEuclid::scales[scaleEncoder.getValue()];
      euclid->randomizePitches();
    }
    if (basePitchEncoder.hasChanged()) {
      euclid->basePitch = basePitchEncoder.getValue();
    }
    if (octavesEncoder.hasChanged()) {
      euclid->octaves = octavesEncoder.getValue();
      euclid->randomizePitches();
    }
    if (trackEncoder.hasChanged()) {
      euclid->mdTrack = trackEncoder.getValue();
    }
  }
};

class MDPitchEuclidSketch : 
public Sketch, public MDCallback, public ClockCallback {
 public:
  MDPitchEuclidConfigPage1 page1;
  MDPitchEuclidConfigPage2 page2;
  MDPitchEuclid pitchEuclid;

 MDPitchEuclidSketch() :page1(&pitchEuclid), page2(&pitchEuclid) {
  }

	void getName(char *n1, char *n2) {
    m_strncpy_p(n1, PSTR("EUC "), 5);
    m_strncpy_p(n2, PSTR("LID "), 5);
  }



  void setup() {
    MidiClock.addOn16Callback(this, (midi_clock_callback_ptr_t)&MDPitchEuclidSketch::on16Callback);
  }

  virtual void show() {
    if (currentPage() == NULL)
      setPage(&page1);
  }

  virtual void doExtra(bool pressed) {
    if (pressed) {
      pitchEuclid.randomizePitches();
    }
  }

  virtual void mute(bool pressed) {
    if (pressed) {
      pitchEuclid.muted = !pitchEuclid.muted;
      if (pitchEuclid.muted) {
				GUI.flash_strings_fill("EUCLID", "MUTED");
      } else {
				GUI.flash_strings_fill("EUCLID", "UNMUTED");
      }
    }
  }

  virtual Page *getPage(uint8_t i) {
    if (i == 0) {
      return &page1;
    } else if (i == 1) {
      return &page2;
    } else {
      return NULL;
    }
  }

  bool handleEvent(gui_event_t *event) {
    if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
      GUI.setPage(&page1);
      return true;
    } 
    else if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
      GUI.setPage(&page2);
      return true;
    } 
    else if (EVENT_PRESSED(event, Buttons.ENCODER1)) {
      pitchEuclid.randomizePitches();
			return true;
    }

		return false;
  }

  void on16Callback(uint32_t counter) {
    pitchEuclid.on16Callback(counter);
  }
};

/* @} @} @} */

#endif /* MDPITCHEUCLIDSKETCH_H__ */
