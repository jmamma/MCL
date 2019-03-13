#include "MCL.h"
#include "PageSelectPage.h"

#define MIX_PAGE 0
#define MUTE_PAGE 1
#define CUE_PAGE 2
#define WAVD_PAGE 8
#define SOUND 7
#define LOUDNESS 10

void PageSelectPage::setup() {}
void PageSelectPage::init() {
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
#endif
  md_exploit.on();
  note_interface.state = true;
}
void PageSelectPage::cleanup() {
  note_interface.init_notes();
}

LightPage *PageSelectPage::get_page(uint8_t page_number, char *str) {
  LightPage *r_page = NULL;
  switch (page_number) {
  case MUTE_PAGE:
    if (str)
      strncpy(str, "MUTE", 5);
    r_page = &mute_page;
    break;
  case MIX_PAGE:
    if (str)
      strncpy(str, "MIX ", 5);
    r_page = &mixer_page;
    break;
  case CUE_PAGE:
    if (str)
      strncpy(str, "CUE ", 5);
    r_page = &cue_page;
    break;
#ifdef WAV_DESIGNER
  case WAVD_PAGE:
    if (str)
      strncpy(str, "WAVD", 5);
    r_page = wd.last_page;
    break;
#endif
  case SOUND:
    if (str)
      strncpy(str, "SOUND", 6);
    r_page = &sound_browser;
    break;
  case LOUDNESS:
    if (str)
      strncpy(str, "LOUDN",6);
    r_page = &loudness_page;
    break;
  default:
    if (str)
      strncpy(str, "----", 4);
  }
  return r_page;
}

uint8_t PageSelectPage::get_nextpage_down() {
  for (int8_t i = page_select - 1; i >= 0; i--) {
    if (get_page(i)) {
      return i;
    }
  }
  return page_select;
}

uint8_t PageSelectPage::get_nextpage_up() {
  for (uint8_t i = page_select + 1; i < 16; i++) {
    if (get_page(i)) {
      return i;
    }
  }
  return page_select;
}
void PageSelectPage::loop() {
  MCLEncoder *enc_ = &enc1;
  // largest_sine_peak = 1.0 / 16.00;
  int dir = 0;
  int16_t newval;
  int8_t diff = enc_->cur - enc_->old;
  if ((diff > 0) && (page_select < 16)) {
    page_select = get_nextpage_up();
  }
  if ((diff < 0) && (page_select > 0)) {
    page_select = get_nextpage_down();
  }

  enc_->cur = 64 + diff;
  enc_->old = 64;
}

void PageSelectPage::display() {
  GUI.setLine(GUI.LINE1);
  char str[6] = "     ";
  get_page(page_select, str);
  LightPage *temp = NULL;
  GUI.put_string_at_fill(0, "Page Select:");
  GUI.setLine(GUI.LINE2);
  GUI.put_string_at_fill(0, str);
}

bool PageSelectPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    uint8_t device = midi_active_peering.get_device(port);

    uint8_t track = event->source - 128;
    // note interface presses are treated as musical notes here
    if (event->mask == EVENT_BUTTON_PRESSED) {
      if (device != DEVICE_MD) {
        return false;
      }
      page_select = track;
      return true;
    }
    if (event->mask == EVENT_BUTTON_RELEASED) {
      if (device != DEVICE_MD) {
        return true;
      }
      //  LightPage *p;
      //  p = get_page(page_select);
      // if (p)
      //  GUI.setPage(p);

      return true;
    }

    return true;
  }
  if (EVENT_RELEASED(event, Buttons.BUTTON2)) {
    LightPage *p;
    p = get_page(page_select);
    if (p) {
      GUI.setPage(p);
    } else {
      md_exploit.off();
      GUI.setPage(&grid_page);
    }
    return true;
  }
  if (EVENT_RELEASED(event, Buttons.ENCODER1) ||
      EVENT_RELEASED(event, Buttons.ENCODER2) ||
      EVENT_RELEASED(event, Buttons.ENCODER3) ||
      EVENT_RELEASED(event, Buttons.ENCODER1)) {
    return true;
  }
  return false;
}

PageSelectPage page_select_page;
