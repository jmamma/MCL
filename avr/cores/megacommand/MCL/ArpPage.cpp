#include "MCL_impl.h"

MCLEncoder arp_oct(0, 3, ENCODER_RES_SEQ);
MCLEncoder arp_mode(0, 17, ENCODER_RES_SEQ);
MCLEncoder arp_rate(0, 4, ENCODER_RES_SEQ);
MCLEncoder arp_enabled(0, 2, ENCODER_RES_SEQ);

MCLEncoder arp_trig(0, 3, ENCODER_RES_SEQ);
MCLEncoder arp_repeat(1, 8, ENCODER_RES_SEQ);
MCLEncoder arp_gate(0, 64, ENCODER_RES_SEQ);
MCLEncoder arp_steps(0, 128, ENCODER_RES_SEQ);

void ArpPage::setup() {
  sub_page = 0;
}

void ArpPage::init() {

  DEBUG_PRINT_FN();
  classic_display = false;
  oled_display.setFont();
  seq_ptc_page.redisplay = true;
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

  arp_oct.cur = arp_track->oct;
  arp_oct.old = arp_oct.cur;

  arp_mode.cur = arp_track->mode;
  arp_mode.old = arp_mode.cur;

  arp_enabled.cur = arp_track->enabled;
  arp_enabled.old = arp_enabled.cur;

  arp_trig.cur = arp_track->trig;
  arp_trig.old = arp_trig.cur;

  arp_repeat.cur = arp_track->repeat;
  arp_repeat.old = arp_repeat.cur;

  arp_gate.cur = arp_track->gate;
  arp_gate.old = arp_gate.cur;

  arp_steps.cur = arp_track->steps;
  arp_steps.old = arp_steps.cur;

}

void ArpPage::config_encoders() {
  if (!sub_page) {
    encoders[0] = &arp_enabled;
    encoders[1] = &arp_mode;
    encoders[2] = &arp_rate;
    encoders[3] = &arp_oct;
  }
  else {
    encoders[0] = &arp_trig;
    encoders[1] = &arp_repeat;
    encoders[2] = &arp_gate;
    encoders[3] = &arp_steps;
  }
}

void ArpPage::cleanup() {
  //  md_exploit.off();
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
#endif
}

void ArpPage::loop() {
 if (seq_ptc_page.md_track_change_check()) {
   track_update();
   return;
 }
 if (!sub_page) {

   if (encoders[0]->hasChanged()) {
      arp_track->enabled = encoders[0]->cur;
      if (encoders[0]->old > 1) {
        seq_ptc_page.note_mask = 0;
        seq_ptc_page.render_arp();
      }
    }
    if (encoders[1]->hasChanged() ||
      encoders[3]->hasChanged()) {
      seq_ptc_page.render_arp();
    }

    if (encoders[2]->hasChanged()) {
      arp_track->set_length(1 << arp_rate.cur);
    }
 }
 else {
   if (encoders[0]->hasChanged()) {
     arp_track->trig = encoders[0]->cur;
   }
   
   if (encoders[1]->hasChanged()) {
     arp_track->repeat = encoders[1]->cur;
   }

   if (encoders[2]->hasChanged()) {
     arp_track->gate = max(1, arp_track->length / encoders[2]->cur);
   }

   if (encoders[3]->hasChanged()) {
     arp_track->steps = encoders[3]->cur;
   }
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

  uint8_t y = 12;
  uint8_t x = 16;


  oled_display.setTextColor(WHITE);
  
  oled_display.setCursor(x, 10);
  oled_display.print("T");
  if (seq_ptc_page.midi_device == &MD) {
    oled_display.print(last_md_track + 1);
  }
  else {
    oled_display.print(last_ext_track + 1);
  }

  oled_display.setCursor(42, 10);

  oled_display.print("ARPEGGIATOR: ");
  oled_display.print(sub_page + 1);
  oled_display.print("/2");

  char str[5];
  if (!sub_page) {
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
    mcl_gui.draw_text_encoder(x + 2 * mcl_gui.knob_w, y, "RATE", str);

    itoa(encoders[3]->cur, str, 10);
    mcl_gui.draw_text_encoder(x + 3 * mcl_gui.knob_w, y, "OCT", str);
  }
  else {

    itoa(encoders[0]->cur, str, 10);
    mcl_gui.draw_text_encoder(x + 0 * mcl_gui.knob_w, y, "TRG", str);
 
    itoa(encoders[1]->cur, str, 10);
    mcl_gui.draw_text_encoder(x + 1 * mcl_gui.knob_w, y, "REP", str);

    itoa(encoders[2]->cur, str, 10);
    mcl_gui.draw_text_encoder(x + 2 * mcl_gui.knob_w, y, "GATE", str);

    itoa(encoders[3]->cur, str, 10);
    mcl_gui.draw_text_encoder(x + 3 * mcl_gui.knob_w, y, "LEN", str);
 
  }

  oled_display.display();
  oled_display.setFont(oldfont);
}

bool ArpPage::handleEvent(gui_event_t *event) {
  if (EVENT_PRESSED(event, Buttons.BUTTON1) ||
      EVENT_PRESSED(event, Buttons.BUTTON2) ||
     // EVENT_PRESSED(event, Buttons.BUTTON3) ||
      EVENT_PRESSED(event, Buttons.BUTTON4)) {
    GUI.ignoreNextEvent(event->source);
    GUI.popPage();
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
    sub_page = !sub_page; 
    config_encoders();
    return true;
  }

  if (arp_track->enabled) {
    seq_ptc_page.handleEvent(event);
  }
  if (note_interface.is_event(event)) {
    uint8_t track = event->source - 128;
    if (midi_active_peering.get_device(event->port)->id != DEVICE_MD) {
      return true;
    }
  }
  return false;
}
