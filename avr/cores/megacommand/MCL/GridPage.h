/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef GRIDPAGE_H__
#define GRIDPAGE_H__

#include "GUI.h"
#include "GridEncoder.h"

class GridPage : public LightPage {
public:
  float frames_fps;
  uint8_t cur_col = 0;
  uint8_t cur_row = 0;
  uint16_t frames = 0;
  uint16_t frames_startclock = 0;
  uint16_t grid_lastclock = 0;
  uint8_t fx_dc = 0;
  uint8_t fx_fb = 0;
  uint8_t fx_lv = 0;
  uint8_t fx_tm = 0;
  uint8_t dispeffect;
  char currentkitName[16];
  uint8_t grid_models[22];
  bool reload_slot_models;

  GridPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
           Encoder *e4 = NULL) : LightPage(e1, e2 ,e3, e4) {
      }
  virtual bool handleEvent(gui_event_t *event);
  void toggle_fx1();
  void toggle_fx2();
  void displayScroll(uint8_t i);
  void displaySlot(uint8_t i);
  void load_slot_models();
  void tick_frames();
  void display();
  void setup();
  void init();
  void loop();
};

void encoder_fx_handle(Encoder *enc);
void encoder_param2_handle(Encoder *enc);

#endif /* GRIDPAGE_H__ */
