#include "MCL_impl.h"

MCLEncoder arp_range(0, 4, ENCODER_RES_SEQ);
MCLEncoder arp_mode(0, 17, ENCODER_RES_SEQ);
MCLEncoder arp_rate(0, 4, ENCODER_RES_SEQ);
MCLEncoder arp_enabled(0, 2, ENCODER_RES_SEQ);

void ArpPage::setup() {
}

void ArpPage::init() {

  DEBUG_PRINT_FN();
  classic_display = false;
  oled_display.setFont();
  seq_ptc_page.display();
  track_update();
}

void ArpPage::track_update() {

  arp_track = &mcl_seq.ext_arp_tracks[last_ext_track];
  if (seq_ptc_page.midi_device == &MD) {
    arp_track = &mcl_seq.md_arp_tracks[last_md_track];
  }

  arp_rate.cur = arp_track->rate;
  arp_rate.old = arp_rate.cur;

  arp_range.cur = arp_track->range;
  arp_range.old = arp_range.cur;

  arp_mode.cur = arp_track->mode;
  arp_mode.old = arp_mode.cur;

  arp_enabled.cur = arp_track->enabled;
  arp_enabled.old = arp_enabled.cur;

  if (last_arp_track && arp_track != last_arp_track) {
    if (last_arp_track->enabled != ARP_LATCH) {
      last_arp_track->clear_notes();
    }
  }
  if (arp_track->enabled != ARP_LATCH) {
  seq_ptc_page.render_arp();
  }
  last_arp_track = arp_track;
}

void ArpPage::cleanup() {
  //  md_exploit.off();
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
#endif
}

void ArpPage::loop() {

 if (encoders[0]->hasChanged()) {
    arp_track->enabled = encoders[0]->cur;
    seq_ptc_page.render_arp(encoders[0]->old != 1);
 }
  if (encoders[1]->hasChanged() ||
      encoders[3]->hasChanged()) {
    arp_track->range = arp_range.cur;
    arp_track->mode = arp_mode.cur;
    seq_ptc_page.render_arp(arp_track->enabled != ARP_LATCH);
  }

  if (encoders[2]->hasChanged()) {
    arp_track->set_length(1 << arp_rate.cur);
    arp_track->rate = arp_rate.cur;
  }

}

typedef char arp_name_t[4];

const arp_name_t arp_names[] PROGMEM = {
    "UP", "DWN", "UD",  "DU", "UND", "DNU", "CNV", "DIV", "CND",
    "PU", "PD", "TU", "TD", "UPP", "DP", "U2",  "D2",  "RND",
};

void ArpPage::display() {

  if (!classic_display) {
  }
  auto oldfont = oled_display.getFont();
  oled_display.setFont(&TomThumb);

  oled_display.fillRect(8, 2, 128 - 16, 32 - 2, BLACK);
  oled_display.drawRect(8 + 1, 2 + 1, 128 - 16 - 2, 32 - 2 - 2, WHITE);

  oled_display.setCursor(42, 10);

  oled_display.setTextColor(WHITE);
  oled_display.print("ARPEGGIATOR: T");

  if (seq_ptc_page.midi_device == &MD) {
    oled_display.print(last_md_track + 1);
  }
  else {
    oled_display.print(last_ext_track + 1);
  }

  char str[5];
  uint8_t y = 12;
  uint8_t x = 16;

  switch (encoders[0]->cur) {
  case ARP_ON:
    strcpy(str, "ON");
    break;
  case ARP_OFF:
    strcpy(str, "--");
    break;
  case ARP_LATCH:
    strcpy(str, "LAT");
    break;
  }
  mcl_gui.draw_text_encoder(x + 0 * mcl_gui.knob_w, y, "ARP", str);

  m_strncpy_p(str, arp_names[encoders[1]->cur], 4);

  mcl_gui.draw_text_encoder(x + 1 * mcl_gui.knob_w, y, "MODE", str);

  mcl_gui.put_value_at(encoders[2]->cur, str);
  mcl_gui.draw_text_encoder(x + 2 * mcl_gui.knob_w, y, "RATE", str);

  mcl_gui.put_value_at(encoders[3]->cur, str);
  mcl_gui.draw_text_encoder(x + 3 * mcl_gui.knob_w, y, "RANGE", str);

  oled_display.display();
  oled_display.setFont(oldfont);
}

bool ArpPage::handleEvent(gui_event_t *event) {
  if (EVENT_PRESSED(event, Buttons.BUTTON1) ||
      EVENT_PRESSED(event, Buttons.BUTTON3) ||
      EVENT_PRESSED(event, Buttons.BUTTON2) ||
      EVENT_PRESSED(event, Buttons.BUTTON4)) {
    GUI.ignoreNextEvent(event->source);
    GUI.popPage();
    return true;
  }
  if (note_interface.is_event(event)) {
    seq_ptc_page.handleEvent(event);
    return true;
  }
/*  if (note_interface.is_event(event) && midi_active_peering.get_device(event->port) == &MD) {
      trig_interface.send_md_leds(TRIGLED_EXCLUSIVE);
      return true;

  } */
  return false;
}
