#include "MCL.h"
#include "PolyPage.h"

void PolyPage::setup() {}

void PolyPage::init() {

  poly_mask = &mcl_cfg.poly_mask;
  DEBUG_PRINT_FN();
  md_exploit.on();
  note_interface.state = true;
#ifdef OLED_DISPLAY
  classic_display = false;
  oled_display.clearDisplay();
  oled_display.setFont();
#endif

}

void PolyPage::cleanup() {
  //  md_exploit.off();
  seq_ptc_page.init_poly();
  note_interface.state = false;
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
#endif
  md_exploit.off();
}

void PolyPage::draw_mask(uint8_t line_number) {
  if (line_number == 0) {
    GUI.setLine(GUI.LINE1);
  } else {
    GUI.setLine(GUI.LINE2);
  }
  /*Initialise the string with blank steps*/
  char str[17] = "----------------";

  for (int i = 0; i < 16; i++) {

#ifdef OLED_DISPLAY
    if (note_interface.notes[i] > 0) {

      oled_display.fillRect(0 + i * 8, 2, 6, 6, WHITE);
    }

    else if (IS_BIT_SET(*poly_mask, i)) {

      oled_display.fillRect(0 + i * 8, 2, 6, 6, BLACK);
      oled_display.drawRect(0 + i * 8, 2, 6, 6, WHITE);

    }

    else {

      oled_display.fillRect(0 + i * 8, 2, 6, 6, BLACK);
      oled_display.drawLine(+i * 8, 5, 5 + (i * 8), 5, WHITE);
    }

#else

    str[i] = (char)219;

    if (!IS_BIT_SET(*poly_mask, i)) {

      str[i] = (char)'-';
    }
    if (note_interface.notes[i] > 0) {

      str[i] = (char)255;
    }
#endif
  }
#ifndef OLED_DISPLAY
  GUI.put_string_at(0, str);
#endif
}

void PolyPage::toggle_mask(uint8_t i) {
  if (IS_BIT_SET(*poly_mask, i)) {
    CLEAR_BIT(*poly_mask, i);
  } else {
    SET_BIT(*poly_mask, i);
  }
}

void PolyPage::display() {

  if (!classic_display) {
    oled_display.clearDisplay();
  }
  GUI.setLine(GUI.LINE2);
  uint8_t x;
  // GUI.put_string_at(12,"Poly");
  GUI.put_string_at(0, "VOICE SELECT ");


  draw_mask(0);
#ifdef OLED_DISPLAY
  LCD.goLine(1);
  LCD.puts(GUI.lines[1].data);
  oled_display.display();
#endif
}

bool PolyPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    uint8_t track = event->source - 128;
    if (midi_active_peering.get_device(event->port) != DEVICE_MD) {
      return true;
    }
    note_interface.draw_notes(0);
    if (event->mask == EVENT_BUTTON_RELEASED) {
      //  if ((encoders[2]->getValue() == 0)) {

      //    toggle_mute(track);
      //// }

      // else {
      toggle_mask(track);
    }
    if (event->mask == EVENT_BUTTON_RELEASED) {
     note_interface.notes[track] = 0;
    }
    //  }
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER1) ||
      EVENT_PRESSED(event, Buttons.ENCODER2) ||
      EVENT_PRESSED(event, Buttons.ENCODER3) ||
      EVENT_PRESSED(event, Buttons.ENCODER4)) {
    GUI.setPage(&grid_page);
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON1) ||
      EVENT_PRESSED(event, Buttons.BUTTON2) ||
      EVENT_PRESSED(event, Buttons.BUTTON3) ||
      EVENT_PRESSED(event, Buttons.BUTTON4)) {
    GUI.popPage();
    return true;
  }


  return false;
}

PolyPage poly_page;
