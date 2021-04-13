/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef WAVDESIGNERPAGE_H__
#define WAVDESIGNERPAGE_H__

#include "MCL.h"

extern void wav_menu_handler();
extern void wav_render();

class WavDesignerPage : public LightPage {
public:
  uint8_t id;
  static uint8_t opt_mode;
  static uint8_t opt_shape;
  static uint8_t last_mode;

  static bool show_menu;

  static MCLEncoder *opt_param1_capture;
  static MCLEncoder *opt_param2_capture;
  WavDesignerPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
                  Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {}

  virtual void init() {
    if (WavDesignerPage::opt_mode < 3) {
      wav_menu_page.menu.enable_entry(2, false);
    }
    show_menu = false;
  }
  virtual void loop();
  virtual void display();
  virtual bool handleEvent(gui_event_t *event);
};

#endif /* WAVDESIGNER_H__ */
