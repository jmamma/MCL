#include "MCL_impl.h"

MCLEncoder arp_oct(0, 3, ENCODER_RES_SEQ);
MCLEncoder arp_mode(0, 17, ENCODER_RES_SEQ);
MCLEncoder arp_speed(0, 4, ENCODER_RES_SEQ);
MCLEncoder arp_und(0, 2, ENCODER_RES_SEQ);

void ArpPage::setup() {
  arp_speed.cur = 1;
}

void ArpPage::init() {

  DEBUG_PRINT_FN();
#ifdef OLED_DISPLAY
  classic_display = false;
  oled_display.setFont();
  seq_ptc_page.redisplay = true;
  seq_ptc_page.display();
#endif
}

void ArpPage::cleanup() {
  //  md_exploit.off();
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
#endif
}

void ArpPage::loop() {
  
  ArpSeqTrack *arp_track;
  
  ArpSeqTrack *arp_track = mcl_seq.ext_arp_tracks[last_ext_track];
  if (seq_ptc_page.midi_device == &MD) {
    ArpSeqTrack = mcl_seq.md_arp_tracks[last_md_track];
  }

  if (encoders[0]->hasChanged()) {
    switch (encoders[0]->cur) {
    case ARP_ON: 
      arp_track.enable = 1;
      break;
    case ARP_OFF:
      arp_track.enable = 0;
      break;
    }
    if (encoders[0]->old > 1) {
    seq_ptc_page.note_mask = 0;
    seq_ptc_page.render_arp();
    }
  }
  if (encoders[1]->hasChanged() ||
      encoders[3]->hasChanged()) {
    seq_ptc_page.render_arp();
  }

  if (encoders[2]->hasChanged) {
    arp_track.set_speed(arp_speed.cur);
  }
}

typedef char arp_name_t[4];

const arp_name_t arp_names[] PROGMEM = {
    "UP", "DWN", "UD",  "DU", "UND", "DNU", "CNV", "DIV", "CND",
    "PU", "PD", "TU", "TD", "UPP", "DP", "U2",  "D2",  "RND",
};

#ifndef OLED_DISPLAY
void ArpPage::display() {
  uint8_t dev_num;
  if (!redisplay) {
    return true;
  }
 GUI.setLine(GUI.LINE1);

  GUI.put_string_at(0, "ARP");
  GUI.put_string_at(4, "MOD");
  GUI.put_string_at(8, "SPD");
  GUI.put_string_at(12,"OCT");

  GUI.setLine(GUI.LINE2);
  char str[5];

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
  GUI.put_string_at(0, str);
  m_strncpy_p(str, arp_names[encoders[1]->cur], 4);
  GUI.put_string_at(4,str);
  GUI.put_value_at2(8, encoders[2]->cur);
  GUI.put_value_at2(12, encoders[3]->cur);
}
#else 
void ArpPage::display() {

  if (!classic_display) {
  }
  auto oldfont = oled_display.getFont();
  oled_display.setFont(&TomThumb);

  oled_display.fillRect(8, 2, 128 - 16, 32 - 2, BLACK);
  oled_display.drawRect(8 + 1, 2 + 1, 128 - 16 - 2, 32 - 2 - 2, WHITE);

  oled_display.setCursor(42, 10);

  oled_display.setTextColor(WHITE);
  oled_display.print("ARPEGGIATOR");
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

  itoa(encoders[2]->cur, str, 10);
  mcl_gui.draw_text_encoder(x + 2 * mcl_gui.knob_w, y, "SPD", str);

  itoa(encoders[3]->cur, str, 10);
  mcl_gui.draw_text_encoder(x + 3 * mcl_gui.knob_w, y, "OCT", str);

  oled_display.display();
  oled_display.setFont(oldfont);
}

#endif
bool ArpPage::handleEvent(gui_event_t *event) {
  if (EVENT_PRESSED(event, Buttons.BUTTON1) ||
      EVENT_PRESSED(event, Buttons.BUTTON3) ||
      EVENT_PRESSED(event, Buttons.BUTTON2) ||
      EVENT_PRESSED(event, Buttons.BUTTON4)) {
    GUI.ignoreNextEvent(event->source);
    GUI.popPage();
    return true;
  }

  seq_ptc_page.handleEvent(event);

  if (note_interface.is_event(event)) {
    uint8_t track = event->source - 128;
    if (midi_active_peering.get_device(event->port)->id != DEVICE_MD) {
      return true;
    }
  }
  return false;
}
