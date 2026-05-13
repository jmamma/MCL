/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MIXERPAGE_H__
#define MIXERPAGE_H__

// #include "Pages.h"
#include "GUI.h"
#include "MCL.h"
#include "../Drivers/DeviceContext.h"
#include "../Midi/midi-common.h"

class MidiDevice;
class SeqTrack;

class MuteSet {
public:
  uint16_t mutes[4];
};

void encoder_level_handle(EncoderParent *enc);

class MixerPage : public LightPage {
public:
  uint8_t level_pressmode = 0;
  uint8_t disp_levels[16];
  uint8_t ext_disp_levels[16];
  bool mute_toggle = 0;
  uint8_t ext_key_down;
  MidiDevice *midi_device;
  uint8_t mixer_device_idx = 0;

  uint8_t display_mode;
  uint8_t first_track;
  uint16_t redraw_mask;
  bool redraw_mutes;
  bool show_mixer_menu;

  bool draw_encoders;

  PageIndex last_page = NULL_PAGE;

  uint8_t preview_mute_set = 255;
  uint8_t load_mute_set = 255;

  // Don't change order
  MuteSet mute_sets[2];
  uint8_t perf_locks[4][4];
  bool load_types[4][2];
  //

  uint8_t get_mute_set(uint8_t key);
  uint8_t default_mixer_param() const;
  MidiDevice *device_for_mixer_idx(uint8_t device_idx) const;
  DeviceContext context_for_mixer_idx(uint8_t device_idx) const;
  DeviceContext selected_mixer_context() const;
  MidiDevice *selected_mixer_device() const;
  void sync_selected_mixer_device();
  void select_mixer_device(uint8_t device_idx);
  uint8_t mixer_track_count() const;
  SeqTrack *mixer_seq_track(uint8_t track) const;
  bool display_mute_mask();
  TrigLEDMode mixer_led_mode() const;
  uint8_t *mixer_meter_levels();
  bool mixer_param_supported_for_held_tracks(uint8_t param);
  uint8_t mixer_param_for_encoder(uint8_t encoder_idx, bool is_md_device);
  bool handle_mixer_encoder_edits(bool is_md_device);

  MixerPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
            Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {
    midi_device = nullptr;
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

  void load_perf_locks(uint8_t state);
  void toggle_or_solo(bool solo = false);
  // Handled in MCLSeq
  void onControlChangeCallback_Midi(uint8_t device_idx, uint8_t track,
                                    uint8_t track_param, uint8_t value);

  uint8_t note_to_trig(uint8_t note_num);
  void track_trig(uint8_t device_idx, uint8_t track_number, uint8_t level);
  void trig(uint8_t track_number);

  virtual bool handleEvent(gui_event_t *event);
  virtual void display();
  virtual void loop();
  virtual void setup();
  virtual void init();
  virtual void cleanup();
};

#endif /* MIXERPAGE_H__ */
