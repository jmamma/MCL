#include "GUI/Pages/Sequencer/ArpPage.h"
#include "KeyInterface.h"
#include "Sequencer/MCLSeq.h"
#include "GUI/Pages/Sequencer/SeqPages.h"
#include "MCLGUI.h"
#include "MCLStrings.h"
#include "SeqTrackUtil.h"

namespace {
DeviceIdx arp_device_idx() {
  return SeqPage::current_device_idx();
}

void set_arp_encoder(Encoder &e, int value) NOINLINE();
void set_arp_encoder(Encoder &e, int value) {
  e.cur = value;
  e.old = value;
}

// Indexed by encoders[0]->cur: ARP_OFF=0, ARP_ON=1, ARP_LATCH=2, ARP_LOCK=3.
const char *const arp_state_labels[] PROGMEM = {mclstr_dash, mclstr_on,
                                                mclstr_lat, mclstr_lck};
// Knob caption per encoder slot 0..3.
const char *const arp_knob_labels[] PROGMEM = {mclstr_arp, mclstr_mode,
                                               mclstr_rate, mclstr_range};
} // namespace

MCLEncoder arp_range(0, 4, ENCODER_RES_SEQ);
MCLEncoder arp_mode(0, 18, ENCODER_RES_SEQ);
MCLEncoder arp_rate(0, 16, ENCODER_RES_SEQ);
MCLEncoder arp_enabled(0, 3, ENCODER_RES_SEQ);

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

  set_arp_encoder(arp_rate, arp_track->length);
  set_arp_encoder(arp_range, arp_track->range);
  set_arp_encoder(arp_mode, arp_track->mode);
  set_arp_encoder(arp_enabled, arp_track->enabled);

  if (re_render) {
    if (last_arp_track && arp_track != last_arp_track) {
      if (!last_arp_track->preserves_note_set()) {
        DEBUG_PRINTLN("clear");
        last_arp_track->clear_notes();
      }
    }
    if (!arp_track->preserves_note_set()) {
      seq_ptc_page.render_arp(true, arp_device_idx(), n);
    }
  }
  last_arp_track = arp_track;
}

void ArpPage::loop() {
  uint8_t n = current_track;

  if (encoders[0]->hasChanged()) {
    arp_track->enabled = encoders[0]->cur;
    if (!arp_track->locks_note_set()) {
      seq_ptc_page.render_arp(encoders[0]->old != ARP_ON, arp_device_idx(), n);
    }
  }
  if (encoders[1]->hasChanged() || encoders[3]->hasChanged()) {
    arp_track->range = arp_range.cur;
    arp_track->mode = arp_mode.cur;
    seq_ptc_page.render_arp(!arp_track->preserves_note_set(),
                            arp_device_idx(), n, true);
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

  for (uint8_t i = 0; i < 4; ++i) {
    uint8_t cur = encoders[i]->cur;
    if (i == 0) {
      strcpy_P(str, (PGM_P)pgm_read_ptr(&arp_state_labels[cur]));
    } else if (i == 1) {
      strncpy_P(str, arp_names[cur], 4);
    } else if (i == 2 && cur == ARP_RATE_TRIG) {
      strcpy_P(str, mclstr_trg);
    } else {
      mcl_gui.put_value_at(cur, str);
    }
    mcl_gui.draw_text_encoder(x + i * mcl_gui.knob_w, y,
                              (PGM_P)pgm_read_ptr(&arp_knob_labels[i]), str,
                              isEncoderFocused(i));
  }
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
