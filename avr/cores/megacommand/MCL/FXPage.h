/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef FXPAGE_H__
#define FXPAGE_H__

#include "GUI.h"
#include "MCLEncoder.h"

#define NUM_FX_PAGES 2
typedef struct fx_param_t {
  uint8_t type;
  uint8_t param;
} fx_param_t;

//params_ 2 dimensional array, consisting of [MD_FX_TYPE][MD_FX_PARAM_NUMBER]
//
class FXPage : public LightPage, MidiCallback {
public:
  FXPage(Encoder *e1 = NULL, Encoder *e2 = NULL,
          Encoder *e3 = NULL, Encoder *e4 = NULL, fx_param_t *params_ = NULL, uint8_t num_of_params_ = 0)
      : LightPage(e1, e2, e3, e4) {

      params = params_;
      num_of_params = num_of_params_;
  }

  bool handleEvent(gui_event_t *event);
  bool midi_state = false;

  fx_param_t *params;
  uint8_t num_of_params;

  bool page_mode;
  uint8_t page_id;

  void display();
  void setup();
  void init();
  void loop();
  void cleanup();

  void update_encoders();

  void setup_callbacks();
  void remove_callbacks();

  void onControlChangeCallback_Midi(uint8_t *msg);
};

extern MCLEncoder fx_page_param1;
extern MCLEncoder fx_page_param2;
extern MCLEncoder fx_page_param3;
extern MCLEncoder fx_page_param4;

#endif /* FXPAGE_H__ */
