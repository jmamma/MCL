#include "MCL_impl.h"

uint8_t WavDesignerPage::opt_mode = 0;
uint8_t WavDesignerPage::opt_shape = 0;
bool WavDesignerPage::show_menu = false;
uint8_t WavDesignerPage::last_mode = 0;

MCLEncoder *WavDesignerPage::opt_param1_capture;
MCLEncoder *WavDesignerPage::opt_param2_capture;

void wav_menu_handler() {
  if (WavDesignerPage::opt_mode == WavDesignerPage::last_mode) {
    if (WavDesignerPage::opt_mode < 3) {
      wd.pages[WavDesignerPage::opt_mode].osc_waveform =
          WavDesignerPage::opt_shape;
    }
  }
  WavDesignerPage::last_mode = WavDesignerPage::opt_mode;
  if (WavDesignerPage::opt_mode == 3) {
    wav_menu_page.menu.enable_entry(1, false);
    wav_menu_page.menu.enable_entry(2, true);
    GUI.setPage(&wd.mixer);
  } else {
    wav_menu_page.menu.enable_entry(1, true);
    wav_menu_page.menu.enable_entry(2, false);
    GUI.setPage(&wd.pages[WavDesignerPage::opt_mode]);
  }
}

void wav_render() { wd.prompt_send(); }

void WavDesignerPage::display() {
  if (show_menu) {
    constexpr uint8_t width = 52;
    oled_display.setFont(&TomThumb);
    oled_display.fillRect(128 - width - 2, 0, width + 2, 32, BLACK);
    wav_menu_page.draw_menu(128 - width, 8, width);
  }
}

void WavDesignerPage::loop() {
  if (show_menu) {
    wav_menu_page.loop();
  }
}

bool WavDesignerPage::handleEvent(gui_event_t *event) {
  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
    opt_param1_capture = (MCLEncoder *)encoders[0];
    opt_param2_capture = (MCLEncoder *)encoders[1];
    encoders[0] = &wav_menu_value_encoder;
    encoders[1] = &wav_menu_entry_encoder;
    show_menu = true;
    if (WavDesignerPage::opt_mode < 3) {
      WavDesignerPage::opt_shape =
          wd.pages[WavDesignerPage::opt_mode].osc_waveform;
    }
    wav_menu_page.init();
    return true;
  }
  if (EVENT_RELEASED(event, Buttons.BUTTON3)) {
    if (show_menu) {
      void (*row_func)();
      row_func =
          wav_menu_page.menu.get_row_function(wav_menu_page.encoders[1]->cur);

      show_menu = false;
      encoders[0] = (Encoder *)opt_param1_capture;
      encoders[1] = (Encoder *)opt_param2_capture;

      if (row_func != NULL) {
        row_func();
      } else {
        wav_menu_handler();
      }
    }
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    GUI.setPage(&page_select_page);
    return true;
  }
  return false;
}
