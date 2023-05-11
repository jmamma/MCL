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

  uint8_t level_pressmode = 0;
  int8_t disp_levels[16];
  int8_t ext_disp_levels[6];

  MidiDevice* midi_device;

  uint8_t display_mode;
  uint8_t first_track;
  uint16_t redraw_mask;
  bool show_mixer_menu;

  uint8_t current_mute_set = 0;
  uint8_t preview_mute_set = 255;
  uint16_t mute_sets[2][4];

  uint8_t get_mute_set(uint8_t key);

  MixerPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
            Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {
        midi_device = &MD;
        memset(mute_sets,0xFF,sizeof(mute_sets));
      }
  void adjust_param(EncoderParent *enc, uint8_t param);

  void draw_levels();
  void set_level(int curtrack, int value);
  void set_display_mode(uint8_t param);
  void disable_record_mutes();
  void oled_draw_mutes();
  void switch_mute_set(uint8_t state);
  void populate_mute_set();

  virtual bool handleEvent(gui_event_t *event);
  virtual void display();
  virtual void loop();
  virtual void setup();
  virtual void init();
  virtual void cleanup();
};

#endif /* MIXERPAGE_H__ */
