/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef PERFPAGE_H__
#define PERFPAGE_H__

#include "GUI.h"
#include "MCLEncoder.h"

#include "PerfEncoder.h"

#define NUM_PERF_CONTROLS 4
#define PERF_DESTINATION NUM_PERF_PARAMS

class PerfPage : public LightPage, MidiCallback {
public:
  PerfPage(Encoder *e1 = NULL, Encoder *e2 = NULL,
          Encoder *e3 = NULL, Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {
  }

  bool learn = false;

  bool midi_state = false;
  uint8_t page_mode;
  uint8_t perf_id;

  PerfEncoder *perf_encoders[4];

  bool handleEvent(gui_event_t *event);

  void draw_dest(uint8_t knob, uint8_t value);
  void draw_param(uint8_t knob, uint8_t  dest, uint8_t param);
  void display();
  void setup();

  void init();
  void loop();
  void cleanup();
  virtual void config_encoders();
  void setup_callbacks();
  void remove_callbacks();

  void onControlChangeCallback_Midi(uint8_t *msg);
  void onControlChangeCallback_Midi2(uint8_t *msg);
};

extern MCLEncoder perf_page_param1;
extern MCLEncoder perf_page_param2;
extern MCLEncoder perf_page_param3;
extern MCLEncoder perf_page_param4;

#endif /* PERFPAGE_H__ */
