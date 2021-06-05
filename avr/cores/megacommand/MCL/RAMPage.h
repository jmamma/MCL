/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef RAMPAGE_H__
#define RAMPAGE_H__

#include "GUI.h"
#include "MCLEncoder.h"
#include "MCLActions.h"

#define NUM_RAM_PAGES 2

class RAMPage : public LightPage, MidiCallback {
public:
  RAMPage(uint8_t _page_id, Encoder *e1 = NULL, Encoder *e2 = NULL,
          Encoder *e3 = NULL, Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {
    page_id = _page_id;
    if (page_id == 0) {
      track1 = 15;
      track2 = 14;
    }
  }

  bool handleEvent(gui_event_t *event);
  bool midi_state = false;
  static bool cc_link_enable;

  static uint8_t rec_states[NUM_RAM_PAGES];
  static uint8_t slice_modes[NUM_RAM_PAGES];

  uint8_t rec_state;
  uint8_t track1;
  uint8_t track2;
  uint8_t page_id;
  uint16_t transition_step;
  uint8_t record_len;

  uint8_t wheel_spin;
  uint16_t wheel_spin_last_clock;
  void display();
  void setup();
  void init();
  void loop();
  void cleanup();
  void setup_ram_rec(uint8_t track, uint8_t model, uint8_t lev, uint8_t source, uint8_t len,
                     uint8_t rate, uint8_t pan, uint8_t linked_track = 255);
  void setup_ram_rec_mono(uint8_t track, uint8_t lev, uint8_t source, uint8_t len,
                          uint8_t rate);
  void setup_ram_rec_stereo(uint8_t track, uint8_t lev, uint8_t source, uint8_t len,
                            uint8_t rate);
  void setup_ram_play(uint8_t track, uint8_t model, uint8_t pan,
                      uint8_t linked_track = 255);

  void setup_ram_play_mono(uint8_t track);
  void setup_ram_play_stereo(uint8_t track);
  void prepare_link(uint8_t track, uint8_t steps, uint8_t row, uint8_t transition = TRANSITION_NORMAL);

  void reverse(uint8_t track);
  bool slice(uint8_t track, uint8_t linked_track);
  void setup_sequencer(uint8_t track);


  void setup_callbacks();
  void remove_callbacks();

  void onControlChangeCallback_Midi(uint8_t *msg);
};

extern MCLEncoder ram_a_param1;
extern MCLEncoder ram_a_param2;
extern MCLEncoder ram_a_param3;
extern MCLEncoder ram_a_param4;

extern MCLEncoder ram_b_param1;
extern MCLEncoder ram_b_param2;
extern MCLEncoder ram_b_param3;
extern MCLEncoder ram_b_param4;

#endif /* RAMPAGE_H__ */
