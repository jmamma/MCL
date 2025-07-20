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


  void draw_dest(uint8_t knob, uint8_t value, bool dest = true);
  void draw_param(uint8_t knob, uint8_t  dest, uint8_t param);

  virtual void display();
  virtual void setup();

  virtual void init();
  virtual void loop();
  virtual void cleanup();
  virtual void config_encoders() {};
  virtual void setup_callbacks() {};
  virtual void remove_callbacks() {};

  virtual bool handleEvent(gui_event_t *event);
};

#endif /* PERFPAGEPARENT_H__ */
