/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MIXERPAGE_H__
#define MIXERPAGE_H__

//#include "Pages.h"
#include "GUI.h"


class MixerMidiEvents : public MidiCallback {
public:
  bool state;

  void setup_callbacks();
  void remove_callbacks();
  uint8_t note_to_trig(uint8_t note_num);
  void onNoteOnCallback_Midi(uint8_t *msg);
  void onNoteOffCallback_Midi(uint8_t *msg);
  void onControlChangeCallback_Midi(uint8_t *msg);
};

void encoder_level_handle(EncoderParent *enc);
void encoder_filtf_handle(EncoderParent *enc);
void encoder_filtw_handle(EncoderParent *enc);
void encoder_filtq_handle(EncoderParent *enc);
void encoder_lastparam_handle(EncoderParent *enc);

class MixerPage : public LightPage {
public:
  MixerMidiEvents midi_events;
  uint8_t params[16][24];

  uint8_t level_pressmode = 0;
  int8_t disp_levels[16];
  char info_line2[9];
  uint8_t display_mode;
  MixerPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
            Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {
      }
  void adjust_param(EncoderParent *enc, uint8_t param);

  void draw_levels();
  void set_level(int curtrack, int value);
  void set_display_mode(uint8_t param);

  virtual bool handleEvent(gui_event_t *event);
  virtual void display();
  virtual void loop();
  virtual void setup();
  virtual void init();
  virtual void cleanup();
};

#endif /* MIXERPAGE_H__ */
