/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef GRIDPAGEPAGE_H__
#define GRIDPAGEPAGE_H__
#include "GUI.h"

class GridPage : LightPage {
public:
  float frames_fps;
  uint8_t cur_col = 0;
  uint8_t cur_row = 0;
  uint16_t frames;
  uint16_t frames_startclock;

  bool reload_slot_models;

  GridPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
           Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {}
  virtual bool handleEvent(gui_event_t *event);
  void toggle_fx1();
  void toggle_fx2();
  void encoder_fx_handle(Encoder *enc);

  void displayScroll(uint8_t i);
  void load_slot_models();

  void loop();
};

#endif /* GRIDPAGEPAGE_H__ */
