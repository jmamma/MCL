/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MELODY_HELPER_SKETCH_H__
#define MELODY_HELPER_SKETCH_H__

#include <MD.h>
#include <GUI.h>

char *melodyHelperNoteNames[] = {
  "C",
  "Db",
  "D",
  "Eb",
  "E",
  "F",
  "Gb",
  "G",
  "Ab",
  "A",
  "Bb",
  "B"
};

#define NOTE_OFFSET 8

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
 * \addtogroup md_sketches_melody_helper MachineDrum Melody Helper Sketch
 *
 * @{
 **/


class MelodyHelperSketch : 
public Sketch, public MidiCallback, public MDCallback {
 public:
  bool muted;

  MelodyHelperSketch() {
    muted = true;
  }
  
  void setup() {
    Midi.addOnControlChangeCallback(this, (void(MidiCallback::*)(uint8_t*))&MelodyHelperSketch::onCCCallback);
  }

  virtual void onCCCallback(uint8_t *msg) {
    if (muted)
      return;
    
    uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
    uint8_t track, param;
    MD.parseCC(channel, msg[1], &track, &param);

    if (param == 0) {
      int8_t offset;
      uint8_t pitch = MD.trackGetCCPitch(track, msg[2], &offset);

      GUI.setLine(GUI.LINE1);
      if (monster) {
				GUI.flash_p_string_at_fill(0, PSTR("TRACK:"));
				GUI.flash_put_value_at(7, track + 1);

				GUI.setLine(GUI.LINE2);
				GUI.flash_p_string_at(0, MD.getMachineName(MD.kit.models[track]));
      } else {
				GUI.put_p_string_at_fill(0, PSTR("TRACK:"));
				GUI.put_value_at(7, track + 1);

				GUI.setLine(GUI.LINE2);
				GUI.put_p_string_at(0, MD.getMachineName(MD.kit.models[track]));
      }

      if (pitch == 128) {
				if (monster) {
					GUI.flash_p_string_at_fill(NOTE_OFFSET, PSTR("XXX"));
				} else {
					GUI.put_p_string_at_fill(NOTE_OFFSET, PSTR("XXX"));
				}
      } else {
        uint8_t note = pitch % 12;
        uint8_t octave = pitch / 12;
				if (monster) {
					GUI.flash_string_at_fill(NOTE_OFFSET, melodyHelperNoteNames[note]);
					GUI.lines[GUI.curLine].flash[NOTE_OFFSET + 1] = octave + '0';
					for (uint8_t i = 0; i < offset; i += 2) {
						GUI.lines[GUI.curLine].flash[NOTE_OFFSET + 2 + (i >> 1)] = '+';
					}
				} else {
					GUI.put_string_at_fill(NOTE_OFFSET, melodyHelperNoteNames[note]);
					GUI.lines[GUI.curLine].data[NOTE_OFFSET + 1] = octave + '0';
					for (uint8_t i = 0; i < offset; i += 2) {
						GUI.lines[GUI.curLine].data[NOTE_OFFSET + 2 + (i >> 1)] = '+';
					}
				}
      }
    }
  }

  void getName(char *n1, char *n2) {
    m_strncpy_p(n1, PSTR("MEL "), 5);
    m_strncpy_p(n2, PSTR("ODY "), 5);
  }

  virtual void show() {
    if (monster) {
      mute(true);
      GUI.setSketch(&_defaultSketch);
    } else {
      muted = false;
    }
  }

  virtual void hide() {
    if (!monster) {
      muted = true; 
      GUI.flash_strings_fill("MELODY", "MUTED");
    }
  }

  virtual void mute(bool pressed) {
    if (pressed) {
      muted = !muted;
      if (muted) {
				GUI.flash_strings_fill("MELODY", "MUTED");
      } else {
				GUI.flash_strings_fill("MELODY", "UNMUTED");
      }
    }
  }

  virtual void doExtra(bool pressed) {
  }

  virtual Page *getPage(uint8_t i) {
    return NULL;
  }
};

/* @} @} @} */

#endif /* MELODY_HELPER_SKETCH_H__ */
