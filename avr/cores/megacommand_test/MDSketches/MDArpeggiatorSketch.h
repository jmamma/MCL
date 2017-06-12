/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MDARPEGGIATORSKETCH_H__
#define MDARPEGGIATORSKETCH_H__

#include <MD.h>
#include <GUI.h>
#include <MidiTools.h>
#include <Arpeggiator.hh>

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
 * \addtogroup md_sketches_arpeggiator MachineDrum Arpeggiator Sketch
 *
 * @{
 **/

class ArpeggiatorSketch : public Sketch, public MDCallback {
public:
  MDArpeggiatorClass arpeggiator;

  class ConfigPage_1 : public EncoderPage {
  public:
    MDArpeggiatorClass *arpeggiator;
    MDMelodicTrackFlashEncoder trackEncoder;
    RangeEncoder speedEncoder;
    RangeEncoder octavesEncoder;
    RangeEncoder lenEncoder;
  
  ConfigPage_1(MDArpeggiatorClass *_arp) :
    trackEncoder("TRK"),
      speedEncoder(1, 16, "SPD"),
      octavesEncoder(0, 5, "OCT"),
      lenEncoder(0, 16, "LEN"),
      arpeggiator(_arp)
  {
    encoders[0] = &trackEncoder;
    encoders[1] = &speedEncoder;
    encoders[2] = &octavesEncoder;
    encoders[3] = &lenEncoder;
    trackEncoder.setValue(arpeggiator->arpTrack);
    speedEncoder.setValue(arpeggiator->arpSpeed);
  }  
  
  virtual void display() {
    EncoderPage::display();
    if (redisplay || lenEncoder.hasChanged()) {
      if (lenEncoder.getValue() == 0)
        GUI.put_p_string_at(12, PSTR("INF"));
    }
  }
  
  virtual void loop() {
    if (lenEncoder.hasChanged()) {
      arpeggiator->arpTimes = lenEncoder.getValue();
    }
    if (speedEncoder.hasChanged()) {
      arpeggiator->arpSpeed = speedEncoder.getValue();
    }
    if (octavesEncoder.hasChanged()) {
      arpeggiator->arpOctaves = octavesEncoder.getValue();
      arpeggiator->arpOctaveCount = 0;
    }
    if (trackEncoder.hasChanged()) {
      uint8_t track = trackEncoder.getValue();
      if (MD.isMelodicTrack(track)) {
        arpeggiator->arpTrack = track;
      }
    }
  }
};

class ConfigPage_2 : public EncoderPage {
  public:
  EnumEncoder styleEncoder;
  EnumEncoder retrigEncoder;
  RangeEncoder retrigSpeedEncoder;
  MDArpeggiatorClass *arpeggiator;

  ConfigPage_2(MDArpeggiatorClass *_arp) :
    styleEncoder(arp_names, (int)ARP_STYLE_CNT, "STY"),
    retrigEncoder(retrig_names, (int)RETRIG_CNT, "TRG"),
      retrigSpeedEncoder(1, 32, "SPD"),
      arpeggiator(_arp)
  {
    encoders[0] = &styleEncoder;
    encoders[2] = &retrigEncoder;
    encoders[3] = &retrigSpeedEncoder;
    styleEncoder.setValue(0);
    arpeggiator->arpStyle = (arp_style_t)styleEncoder.getValue();
    retrigEncoder.setValue(0);
    arpeggiator->arpRetrig = (arp_retrig_type_t)retrigEncoder.getValue();
    arpeggiator->calculateArp();
  }
  
  virtual void loop() {
    bool changed = false;
    if (styleEncoder.hasChanged()) {
      arpeggiator->arpStyle = (arp_style_t)styleEncoder.getValue();
      changed = true;
    }
    if (retrigEncoder.hasChanged()) {
      arpeggiator->arpRetrig = (arp_retrig_type_t)retrigEncoder.getValue();
      changed = true;
    }
    if (changed) {
      arpeggiator->calculateArp();
    }
    if (retrigSpeedEncoder.hasChanged()) {
      arpeggiator->retrigSpeed = retrigSpeedEncoder.getValue();
    }
  }
};


  ConfigPage_1 configPage_1;
  ConfigPage_2 configPage_2;

 ArpeggiatorSketch() : configPage_1(&arpeggiator), configPage_2(&arpeggiator) {
  }

  void getName(char *n1, char *n2) {
    m_strncpy_p(n1, PSTR("MD  "), 5);
    m_strncpy_p(n2, PSTR("ARP "), 5);
  }


  void setup() {
    arpeggiator.setup();
  }

  virtual void show() {
    if (currentPage() == NULL)
      setPage(&configPage_1);
  }

  virtual void mute(bool pressed) {
    if (pressed) {
      arpeggiator.muted = !arpeggiator.muted;
      if (arpeggiator.muted) {
	GUI.flash_strings_fill("ARP", "MUTED");
      } else {
	GUI.flash_strings_fill("ARP", "UNMUTED");
      }
    }
  }
  
  virtual Page *getPage(uint8_t i) {
    if (i == 0) {
      return &configPage_1;
    } else if (i == 1) {
      return &configPage_2;
    } else {
      return NULL;
    }
  }

  bool handleEvent(gui_event_t *event) {
    if (currentPage() == &configPage_1) {
      if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
	setPage(&configPage_2);
	return true;
      }
    } else if (currentPage() == &configPage_2) {
      if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
	setPage(&configPage_1);
	return true;
      }
    }

    if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
      arpeggiator.startRecording();
    }
  }

};

/* @} @} @} */

#endif /* MDARPEGGIATORSKETCH_H__ */
