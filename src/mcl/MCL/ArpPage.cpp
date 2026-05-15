#include "ArpPage.h"
#include "KeyInterface.h"
#include "MCLSeq.h"
#include "SeqPages.h"
#include "MCLGUI.h"
#include "MCLStrings.h"
#include "SeqTrackUtil.h"

namespace {
DeviceIdx arp_device_idx() {
  return SeqPage::current_device_idx();
}
} // namespace

MCLEncoder arp_range(0, 4, ENCODER_RES_SEQ);
MCLEncoder arp_mode(0, 18, ENCODER_RES_SEQ);
MCLEncoder arp_rate(1, 16, ENCODER_RES_SEQ);
MCLEncoder arp_enabled(0, 2, ENCODER_RES_SEQ);

void ArpPage::setup() {
  param_select = 255;
}

void ArpPage::init() {
  DEBUG_PRINT_FN();
//  seq_ptc_page.display();
  track_update();
#ifdef PLATFORM_TBD
  seq_ptc_page.send_tbd_keyboard_leds();
#else
  key_interface.send_md_leds(TRIGLED_EXCLUSIVE);
#endif
}

void ArpPage::track_update(uint8_t n, bool re_render) {

  DeviceIdx device_idx = arp_device_idx();
  if (device_idx == DeviceIdx::Primary) {
    if (n >= SeqTrackUtil::track_count(DeviceIdx::Primary)) {
      n = last_primary_track;
    }
    arp_track = &SeqTrackUtil::arp_track(DeviceIdx::Primary, n);
  } else {
    n = last_ext_track;
    arp_track = &SeqTrackUtil::arp_track(DeviceIdx::Secondary, n);
  }

  current_track = n;

  arp_rate.cur = arp_track->length;
  arp_rate.old = arp_rate.cur;

  arp_range.cur = arp_track->range;
  arp_range.old = arp_range.cur;

  arp_mode.cur = arp_track->mode;
  arp_mode.old = arp_mode.cur;

  arp_enabled.cur = arp_track->enabled;
  arp_enabled.old = arp_enabled.cur;

  if (re_render) {
    if (last_arp_track && arp_track != last_arp_track) {
      if (last_arp_track->enabled != ARP_LATCH) {
        DEBUG_PRINTLN("clear");
        last_arp_track->clear_notes();
      }
    }
    if (arp_track->enabled != ARP_LATCH) {
      seq_ptc_page.render_arp(true, arp_device_idx(), n);
    }
  }
  last_arp_track = arp_track;
}

void ArpPage::loop() {
  uint8_t n = current_track;

  if (encoders[0]->hasChanged()) {
    arp_track->enabled = encoders[0]->cur;
    seq_ptc_page.render_arp(encoders[0]->old != 1, arp_device_idx(), n);
  }
  if (encoders[1]->hasChanged() || encoders[3]->hasChanged()) {
    arp_track->range = arp_range.cur;
    arp_track->mode = arp_mode.cur;
    seq_ptc_page.render_arp(arp_track->enabled != ARP_LATCH,
                            arp_device_idx(), n);
  }

  if (encoders[2]->hasChanged()) {
    arp_track->set_length(arp_rate.cur);
  }
}



void ArpPage::display() {

  oled_display.setFont(&TomThumb);

  oled_display.fillRect(8, 2, 128 - 16, 32 - 2, BLACK);
  oled_display.drawRect(8 + 1, 2 + 1, 128 - 16 - 2, 32 - 2 - 2, WHITE);

  oled_display.setCursor(42, 10);

  oled_display.setTextColor(WHITE);
  mcl_print_P(mclstr_arpeggiator);

  if (arp_device_idx() == DeviceIdx::Primary) {
    oled_display.print(current_track + 1);
  } else {
    oled_display.print(last_ext_track + 1);
  }

  char str[5];
  uint8_t y = 12;
  uint8_t x = 16;

  switch (encoders[0]->cur) {
  case ARP_ON:
    strcpy_P(str, mclstr_on);
    break;
  case ARP_OFF:
    strcpy_P(str, mclstr_dash);
    break;
  case ARP_LATCH:
    strcpy_P(str, mclstr_lat);
    break;
  }

  mcl_gui.draw_text_encoder(x + 0 * mcl_gui.knob_w, y, mclstr_arp, str, param_select == 0);

  strncpy_P(str, arp_names[encoders[1]->cur], 4);

  mcl_gui.draw_text_encoder(x + 1 * mcl_gui.knob_w, y, mclstr_mode, str, param_select == 1);

  mcl_gui.put_value_at(encoders[2]->cur, str);
  mcl_gui.draw_text_encoder(x + 2 * mcl_gui.knob_w, y, mclstr_rate, str, param_select == 2);

  mcl_gui.put_value_at(encoders[3]->cur, str);
  mcl_gui.draw_text_encoder(x + 3 * mcl_gui.knob_w, y, mclstr_range, str, param_select == 3);

}

bool ArpPage::handleEvent(gui_event_t *event) {
  if (EVENT_NOTE(event)) {
    return seq_ptc_page.handleEvent(event);
  }

  if (EVENT_CMD(event)) {
    uint8_t key = event->source;
    if (event->mask == EVENT_BUTTON_PRESSED) {

      switch (key) {
      case MDX_KEY_YES:
      case MDX_KEY_NO:
        goto exit;
      }
      if (param_select == 255) { param_select = 0; return true; }
      switch (key) {
       case MDX_KEY_LEFT: {
        if (param_select > 0) {
          param_select--;
        }
        return true;
      }
      case MDX_KEY_RIGHT: {
        if (param_select < 3) {
          param_select++;
        }
        return true;
      }
      case MDX_KEY_UP: {
        encoders[param_select]->cur++;
        return true;
      }
      case MDX_KEY_DOWN: {
        encoders[param_select]->cur--;
        return true;
      }
      }
    }
  }

  if (EVENT_BUTTON(event)) {
    if (EVENT_PRESSED(event, Buttons.BUTTON1) ||
        EVENT_PRESSED(event, Buttons.BUTTON3) ||
        EVENT_PRESSED(event, Buttons.BUTTON2) ||
        EVENT_PRESSED(event, Buttons.BUTTON4)) {
        GUI.ignoreNextEvent(event->source);
    exit:
      mcl.popPage();
      return true;
    }
  }
  return false;
}
