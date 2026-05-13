/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef PERFPAGEPARENT_H__
#define PERFPAGEPARENT_H__

#include "../Drivers/DeviceContext.h"
#include "GUI.h"
#include "MCLEncoder.h"
#include "midi-common.h"

class PerfPageParent : public MidiCallback {
public:

  PerfPageParent() { };

  bool learn = false;
  bool midi_state = false;


  void draw_dest(uint8_t knob, uint8_t value, bool dest = true,
                 DeviceIdx device_idx = DeviceIdx::None);
  void draw_param(uint8_t knob, uint8_t dest, uint8_t param,
                  DeviceIdx device_idx = DeviceIdx::None);

  void setup();

  void init();
  void cleanup();
  virtual void config_encoders() {};

  bool handleEvent(gui_event_t *event);
};

#endif /* PERFPAGEPARENT_H__ */
