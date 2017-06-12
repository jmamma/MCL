/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MD_NOTES_RECORD_SKETCH_H__
#define MD_NOTES_RECORD_SKETCH_H__

#include <MD.h>
#include <MidiTools.h>
#include <MDRecorder.h>

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
 * \addtogroup md_sketches_notes_recorder MachineDrum Notes Recorder Sketch
 *
 * @{
 **/

class MDNotesRecordSketch : 
public Sketch, public MDCallback, public ClockCallback, public MidiCallback {
public:
class ConfigPage_1 : 
public EncoderPage {
public:
  MDMelodicTrackFlashEncoder trackEncoder;
  uint8_t track;

  ConfigPage_1() :
  trackEncoder("TRK")
  {
    track = 255;
    encoders[0] = &trackEncoder;
  }  

  virtual void loop() {
    if (trackEncoder.hasChanged()) {
      track = trackEncoder.getValue();
    }
  }
};

  ConfigPage_1 configPage_1;
  uint8_t track;
  bool muted;

  MDNotesRecordSketch() {
    track = 255;
    muted = false;
  }

  void setup() {
    MDRecorder.setup();
  
    MDTask.addOnKitChangeCallback(this, (md_callback_ptr_t)&MDNotesRecordSketch::onKitChanged);

    MidiClock.addOn16Callback(this, (midi_clock_callback_ptr_t)&MDNotesRecordSketch::on16Callback);
    Midi2.addOnNoteOnCallback(this, (midi_callback_ptr_t)&MDNotesRecordSketch::onNoteOnCallbackKeyboard);
  }

  void destroy() {
  }

  void onNoteOnCallbackKeyboard(uint8_t *msg) {
    if (configPage_1.track != 255) {
      if (!muted) {
	MD.sendNoteOn(configPage_1.track, msg[1], msg[2]);
      }
    }
  }

  bool handleEvent(gui_event_t *event) {
    /*
    if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
      MDRecorder.startRecord(16, 16);
      return true;
    }
    if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
      MDRecorder.looping = true;
      MDRecorder.startPlayback(16);
      return true;
    }
    if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
      MDRecorder.stopPlayback();
      return true;
    }
    if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
      MDRecorder.looping = false;
      MDRecorder.startMDPlayback(16);
      return true;
    }
    */
    if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
      int i = showModalGui("REC FUNCTION", "REC PLY STP MD  ",
			   ALL_ENCODER_MASK,
			   _BV(ButtonsClass::BUTTON1));
      switch (i) {
      case ButtonsClass::ENCODER1:
	MDRecorder.startRecord(16, 16);
	break;
      case ButtonsClass::ENCODER2:
	MDRecorder.looping = true;
	MDRecorder.startPlayback(16);
	break;
      case ButtonsClass::ENCODER3:
	MDRecorder.stopPlayback();
	break;
      case ButtonsClass::ENCODER4:
	MDRecorder.looping = false;
	MDRecorder.startMDPlayback(16);
	break;
      }
    }
    return false;
  }


  void on16Callback() {
    if (isVisible() && currentPage() == &configPage_1) {
      GUI.setLine(GUI.LINE1);
      GUI.put_value(1, MDRecorder.rec16th_counter);
      GUI.put_value(2, MDRecorder.play16th_counter);
      GUI.put_value(3, MDRecorder.eventList.size());
      GUI.setLine(GUI.LINE2);
      GUI.put_value(2, (uint8_t)(MidiClock.div16th_counter % 32));
    }
  }

  void onKitChanged() {
    configPage_1.trackEncoder.old = 255;
  }

  void getName(char *n1, char *n2) {
    m_strncpy_p(n1, PSTR("MD  "), 5);
    m_strncpy_p(n2, PSTR("REC "), 5);
  }

  virtual void show() {
    if (currentPage() == NULL)
      setPage(&configPage_1);
  }

  virtual void mute(bool pressed) {
    if (pressed) {
      muted = !muted;
      MDRecorder.muted = muted;
      if (muted) {
	GUI.flash_strings_fill("MD RECORDER", "MUTED");
      } else {
	GUI.flash_strings_fill("MD RECORDER", "UNMUTED");
      }
    }
  }

  virtual void doExtra(bool pressed) {
    GUI.flash_strings_fill("RECORD", "STARTED");
    MDRecorder.startRecord(16, 16);
  }

  virtual Page *getPage(uint8_t i) {
    if (i == 0) {
      return &configPage_1;
    } else {
      return NULL;
    }
  }
  
};

/* @} @} @} */

#endif /* MD_NOTES_RECORD_SKETCH_H__ */
