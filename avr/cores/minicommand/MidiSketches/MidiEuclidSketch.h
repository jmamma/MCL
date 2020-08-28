#ifndef MIDI_EUCLID_SKETCH_H__
#define MIDI_EUCLID_SKETCH_H__

#include <PitchEuclid.h>

class PitchEuclidConfigPage1 : 
public EncoderPage {
public:
  RangeEncoder pitchLengthEncoder;
  RangeEncoder pulseEncoder;
  RangeEncoder lengthEncoder;
  RangeEncoder offsetEncoder;
	PitchEuclid *euclid;

  PitchEuclidConfigPage1(PitchEuclid *_euclid) :
  pitchLengthEncoder(1, 32, "PTC", 4),
  pulseEncoder(1, 32, "PLS", 3),
  lengthEncoder(2, 32, "LEN", 8),
		offsetEncoder(0, 32, "OFF", 0),
		euclid(_euclid) {
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

class PitchEuclidConfigPage2 : 
public EncoderPage {
public:
  MidiTrackEncoder trackEncoder;
  RangeEncoder scaleEncoder;
  RangeEncoder octavesEncoder;
  NotePitchEncoder basePitchEncoder;
	PitchEuclid *euclid;

  PitchEuclidConfigPage2(PitchEuclid *_euclid) :
  trackEncoder("TRK", 0),
		scaleEncoder(0, PitchEuclid::NUM_SCALES - 1, "SCL", 0),
  basePitchEncoder("BAS"),
		octavesEncoder(0, 4, "OCT"),
		euclid(_euclid)
  {
    encoders[0] = &trackEncoder;
    encoders[3] = &basePitchEncoder;
    encoders[2] = &octavesEncoder;
    encoders[1] = &scaleEncoder;
  }

  void loop() {
    if (scaleEncoder.hasChanged()) {
      euclid->currentScale = (scale_t *)PitchEuclid::scales[scaleEncoder.getValue()];
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

class PitchEuclidSketch : 
public Sketch, public ClockCallback {
public:
  PitchEuclidConfigPage1 page1;
  PitchEuclidConfigPage2 page2;
	PitchEuclid pitchEuclid;

 PitchEuclidSketch() : page1(&pitchEuclid), page2(&pitchEuclid) {
  }

  void setup() {
    pitchEuclid.setup();
    setPage(&page1);
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
    }
  }

    void getName(char *n1, char *n2) {
    m_strncpy_p(n1, PSTR("MID "), 5);
    m_strncpy_p(n2, PSTR("EUC "), 5);
  }

  virtual void show() {
    if (currentPage() == NULL)
      setPage(&page1);
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

  virtual void doExtra(bool pressed) {
    if (pressed) {
      pitchEuclid.randomizePitches();
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


};

#endif /* MIDI_EUCLID_SKETCH_H__ */
