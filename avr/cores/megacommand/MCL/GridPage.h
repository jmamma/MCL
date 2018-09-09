/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef GRIDPAGE_H__
#define GRIDPAGE_H__

#include "GUI.h"
#include "GridEncoder.h"
#include "GridRowHeader.h"

#define MAX_VISIBLE_ROWS 4
#define MAX_VISIBLE_COLS 8

class GridPage : public LightPage {
public:

  GridRowHeader row_headers[MAX_VISIBLE_ROWS];

  float frames_fps;
  uint8_t cursor_x = 0;
  uint8_t cursoy_y = 0;
  uint8_t col = 0;
  uint8_t row = 0;
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
  bool reload_slot_models;

  GridPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
           Encoder *e4 = NULL) : LightPage(e1, e2 ,e3, e4) {
      }
  virtual bool handleEvent(gui_event_t *event);
  void toggle_fx1();
  void toggle_fx2();
  void displayScroll(uint8_t i);
  void displaySlot(uint8_t i);
  uint8_t getCol();
  uint8_t getRow();
  void load_slot_models();
  void load_slot_models_oled();
  void load_slot_model_row(uint8_t y, uint8_t row);
  void shift_slot_models(uint8_t count, bool direction);
  void tick_frames();
  void display();
  void display_oled();
  void display_slot_oled(uint8_t x, uint8_t y, char *strn);
  void setup();
  void cleanup();
  void init();
  void prepare();
  void loop();
};

void encoder_fx_handle(Encoder *enc);
void encoder_param2_handle(Encoder *enc);

#endif /* GRIDPAGE_H__ */
