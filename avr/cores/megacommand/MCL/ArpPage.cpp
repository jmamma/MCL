#include "ArpPage.h"
#include "MCL.h"

MCLEncoder arp_oct(0, 3, ENCODER_RES_SEQ);
MCLEncoder arp_mode(0, 7, ENCODER_RES_SEQ);
MCLEncoder arp_speed(0, 4, ENCODER_RES_SEQ);
MCLEncoder arp_und(0, 1, ENCODER_RES_SEQ);

void ArpPage::setup() {}

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
#define ARP_ON 1
#define ARP_OFF 0

void ArpPage::loop() {
  if (encoders[0]->hasChanged()) {
    switch (encoders[0]->cur) {
    case ARP_ON:
      seq_ptc_page.setup_arp();
      break;
    case ARP_OFF:
      seq_ptc_page.remove_arp();
      break;
    }
  }
}

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
  }
  mcl_gui.draw_text_encoder(x + 0 * mcl_gui.knob_w, y, "ARP", str);

  switch (encoders[1]->cur) {
  case ARP_UP:
    strcpy(str, "UP");
    break;
  case ARP_DOWN:
    strcpy(str, "DWN");
    break;
  case ARP_CIRC:
    strcpy(str, "CIR");
    break;
  case ARP_RND:
    strcpy(str, "RND");
    break;
  case ARP_UPTHUMB:
    strcpy(str, "UPT");
    break;
  case ARP_DOWNPINK:
    strcpy(str, "DNP");
    break;
  case ARP_UPPINK:
    strcpy(str, "UPP");
    break;
  case ARP_DOWNTHUMB:
    strcpy(str, "DNT");
    break;
  default:
    break;
  }
  mcl_gui.draw_text_encoder(x + 1 * mcl_gui.knob_w, y, "MODE", str);

  itoa(encoders[2]->cur, str, 10);
  mcl_gui.draw_text_encoder(x + 2 * mcl_gui.knob_w, y, "SPD", str);

  itoa(encoders[3]->cur, str, 10);
  mcl_gui.draw_text_encoder(x + 3 * mcl_gui.knob_w, y, "OCT", str);

  oled_display.display();
  oled_display.setFont(oldfont);
}

bool ArpPage::handleEvent(gui_event_t *event) {
  if (EVENT_PRESSED(event, Buttons.BUTTON1) || EVENT_PRESSED(event, Buttons.BUTTON3) || EVENT_PRESSED(event, Buttons.BUTTON2) ||
      EVENT_PRESSED(event, Buttons.BUTTON4)) {
    GUI.ignoreNextEvent(event->source);
    GUI.popPage();
    return true;
  }

  seq_ptc_page.handleEvent(event);

  if (note_interface.is_event(event)) {
    uint8_t track = event->source - 128;
    if (midi_active_peering.get_device(event->port) != DEVICE_MD) {
      return true;
    }
  }
  return false;
}
