#include "MCL_impl.h"

#ifdef WAV_DESIGNER

uint8_t WavDesignerPage::opt_mode = 0;
uint8_t WavDesignerPage::opt_shape = 0;
bool WavDesignerPage::show_menu = false;
uint8_t WavDesignerPage::last_mode = 0;

MCLEncoder *WavDesignerPage::opt_param1_capture;
MCLEncoder *WavDesignerPage::opt_param2_capture;

void wavdesign_menu_handler() {
  if (WavDesignerPage::opt_mode == WavDesignerPage::last_mode) {
    if (WavDesignerPage::opt_mode < 3) {
      wd.pages[WavDesignerPage::opt_mode].osc_waveform =
          WavDesignerPage::opt_shape;
    }
  }
  WavDesignerPage::last_mode = WavDesignerPage::opt_mode;
  if (WavDesignerPage::opt_mode == 3) {
    mcl.setPage(WD_MIXER_PAGE);
  } else {
    mcl.setPage((PageIndex) (WD_PAGE_0 + WavDesignerPage::opt_mode));
  }
}

void wav_render() { wd.prompt_send(); }

void WavDesignerPage::display() {
  if (show_menu) {
    constexpr uint8_t width = 52;
    oled_display.setFont(&TomThumb);
    oled_display.fillRect(128 - width - 2, 0, width + 2, 32, BLACK);
    wavdesign_menu_page.draw_menu(128 - width, 8, width);
  }
}

void WavDesignerPage::loop() {
  if (show_menu) {
    wavdesign_menu_page.loop();
  }
}

bool WavDesignerPage::handleEvent(gui_event_t *event) {
  if (EVENT_CMD(event)) {
    uint8_t key = event->source - 64;
    if (event->mask == EVENT_BUTTON_PRESSED) {
      uint8_t inc = 1;
    //  if (show_menu) {
        switch (key) {
        case MDX_KEY_DOWN:
          encoders[1]->cur += inc;
          break;
        case MDX_KEY_UP:
          encoders[1]->cur -= inc;
          break;
        case MDX_KEY_LEFT:
          encoders[0]->cur -= inc;
          break;
        case MDX_KEY_RIGHT:
          encoders[0]->cur += inc;
          break;
        }
     // }
    }
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
    opt_param1_capture = (MCLEncoder *)encoders[0];
    opt_param2_capture = (MCLEncoder *)encoders[1];
    encoders[0] = &wavdesign_menu_value_encoder;
    encoders[1] = &wavdesign_menu_entry_encoder;
    show_menu = true;
    if (WavDesignerPage::opt_mode < 3) {
      WavDesignerPage::opt_shape =
          wd.pages[WavDesignerPage::opt_mode].osc_waveform;
    }
    wavdesign_menu_page.init();
    return true;
  }
  if (EVENT_RELEASED(event, Buttons.BUTTON3)) {
    if (show_menu) {
      void (*row_func)();
      row_func = wavdesign_menu_page.menu.get_row_function(
          wavdesign_menu_page.encoders[1]->cur);

      show_menu = false;
      encoders[0] = (Encoder *)opt_param1_capture;
      encoders[1] = (Encoder *)opt_param2_capture;

      if (row_func != NULL) {
        row_func();
      } else {
        wavdesign_menu_handler();
      }
    }
    return true;
  }
  return false;
}

#endif
