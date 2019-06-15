/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef RAMPAGE_H__
#define RAMPAGE_H__

#include "MCLEncoder.h"
#include "GUI.h"

#define SLOT_RAM_RECORD (1 << (sizeof(GridChain::row) * 8)) - 1 - 1
#define SLOT_RAM_PLAY (1 << (sizeof(GridChain::row) * 8)) - 1 - 2

class RAMPage : public LightPage, MidiCallback {
public:
  RAMPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
            Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {
      }

  bool handleEvent(gui_event_t *event);
  bool midi_state = false;
  uint8_t magic;

  void display();
  void setup();
  void init();
  void cleanup();
  void setup_ram_rec(uint8_t track, uint8_t model, uint8_t mlev, uint8_t len, uint8_t rate, uint8_t pan, uint8_t linked_track = 255);
  void setup_ram_rec_mono(uint8_t track, uint8_t mlev, uint8_t len, uint8_t rate);
  void setup_ram_rec_stereo(uint8_t track, uint8_t mlev, uint8_t len, uint8_t rate);
  void setup_ram_play(uint8_t track, uint8_t model, uint8_t pan, uint8_t linked_track = 255);

  void setup_ram_play_mono(uint8_t track);
  void setup_ram_play_stereo(uint8_t track);

  void reverse(uint8_t track);
  bool slice(uint8_t track, uint8_t linked_track);
  void setup_sequencer(uint8_t track);

  void setup_callbacks();
  void remove_callbacks();

  void onControlChangeCallback_Midi(uint8_t *msg);
};

extern MCLEncoder ram_param1;
extern MCLEncoder ram_param2;
extern MCLEncoder ram_param3;
extern MCLEncoder ram_param4;

#endif /* RAMPAGE_H__ */
