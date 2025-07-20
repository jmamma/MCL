/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MIXERPAGE_H__
#define MIXERPAGE_H__

// #include "Pages.h"
#include "GUI.h"
#include "MCL.h"
#include "MD.h"

class MuteSet {
public:
  uint16_t mutes[4];
};

class MixerMidiEvents : public MidiCallback {
public:
  bool state;

  void setup_callbacks();
  void remove_callbacks();
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
  uint8_t level_pressmode = 0;
  uint8_t disp_levels[16];
  uint8_t ext_disp_levels[6];
  bool mute_toggle = 0;
  uint8_t ext_key_down;
  MidiDevice *midi_device;

  uint8_t display_mode;
  uint8_t first_track;
  uint16_t redraw_mask;
  bool redraw_mutes;
  bool show_mixer_menu;

  bool draw_encoders;

  PageIndex last_page = NULL_PAGE;

  uint8_t current_mute_set = 255;
  uint8_t preview_mute_set = 255;
  uint8_t load_mute_set = 255;

  void send_fx(uint8_t param, Encoder *enc, uint8_t type);

  // Don't change order
  MuteSet mute_sets[2];
  uint8_t perf_locks[4][4];
  bool load_types[4][2];
  //

  uint8_t get_mute_set(uint8_t key);

  MixerPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
            Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {
    midi_device = &MD;
    memset(mute_sets, 0xFF, sizeof(mute_sets) + sizeof(perf_locks));
    //memset(perf_locks, 0xFF, sizeof(perf_locks));
    memset(load_types, 1, sizeof(load_types));
  }
  void adjust_param(EncoderParent *enc, uint8_t param);
  void draw_levels();
  void draw_encs();
  void redraw();
  void set_level(int curtrack, int value);
  void set_display_mode(uint8_t param);

  void record_mutes_set(bool state);
  void disable_record_mutes(bool clear = false);
  void oled_draw_mutes();
  void switch_mute_set(uint8_t state, bool load_perf = false, bool *load_types = nullptr);
  void populate_mute_set();

  void load_perf_locks(uint8_t state);
  void toggle_or_solo(bool solo = false);
  // Handled in MCLSeq
  void onControlChangeCallback_Midi(uint8_t track, uint8_t track_param,
                                    uint8_t value);

  uint8_t note_to_trig(uint8_t note_num);
  void trig(uint8_t track_number) {
    disp_levels[track_number] = MD.kit.levels[track_number];
    if (MD.kit.trigGroups[track_number] < 16) {
      disp_levels[MD.kit.trigGroups[track_number]] =
          MD.kit.levels[MD.kit.trigGroups[track_number]];
    }
  }

  virtual bool handleEvent(gui_event_t *event);
  virtual void display();
  virtual void loop();
  virtual void setup();
  virtual void init();
  virtual void cleanup();
};

#endif /* MIXERPAGE_H__ */
