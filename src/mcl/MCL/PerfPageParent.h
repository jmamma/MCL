/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef PERFPAGEPARENT_H__
#define PERFPAGEPARENT_H__

#include "GUI.h"
#include "MCLEncoder.h"
#include "midi-common.h"

class PerfPageParent : public MidiCallback {
public:

  PerfPageParent() { };

  bool learn = false;
  bool midi_state = false;


  void draw_dest(uint8_t knob, uint8_t value, bool dest = true,
                 uint8_t device_slot = 0);
  void draw_param(uint8_t knob, uint8_t dest, uint8_t param,
                  uint8_t device_slot = 0);

  void setup();

  void init();
  void cleanup();
  virtual void config_encoders() {};

  bool handleEvent(gui_event_t *event);
};

#endif /* PERFPAGEPARENT_H__ */
