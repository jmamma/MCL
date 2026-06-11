/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef WAVDESIGNERPAGE_H__
#define WAVDESIGNERPAGE_H__

#include "MCL.h"
#include "MCLGUI.h"
#include "GUI/Pages/Sequencer/SeqPages.h"

extern void wavdesign_menu_handler();
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

  void init() override {
    if (WavDesignerPage::opt_mode < 3) {
      wavdesign_menu_page.menu.enable_entry(2, false);
    }
    show_menu = false;
  }
  void cleanup() override = 0;
  void loop() override = 0;
  void display() override = 0;
  bool handleEvent(gui_event_t *event) override;
#if defined(MCL_HAS_DESKTOP_MOUSE)
  bool handleMouseEvent(mcl_mouse_event_t *event) override;
#endif
};

#endif /* WAVDESIGNER_H__ */
