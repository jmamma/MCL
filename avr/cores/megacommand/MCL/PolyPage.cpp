#include "MCL_impl.h"

void PolyPage::setup() {}

void PolyPage::init() {

  poly_mask = &mcl_cfg.poly_mask;
  DEBUG_PRINT_FN();
  trig_interface.on();
  note_interface.init_notes();
#ifdef OLED_DISPLAY
  classic_display = false;
  oled_display.clearDisplay();
  oled_display.setFont();
#endif
}

void PolyPage::cleanup() {
  seq_ptc_page.init_poly();
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
#endif
  trig_interface.off();
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
    if (note_interface.is_note(i)) {

      oled_display.fillRect(0 + i * 8, 2, 6, 6, WHITE);
    }

    else if (IS_BIT_SET16(*poly_mask, i)) {

      oled_display.fillRect(0 + i * 8, 2, 6, 6, BLACK);
      oled_display.drawRect(0 + i * 8, 2, 6, 6, WHITE);

    }

    else {

      oled_display.fillRect(0 + i * 8, 2, 6, 6, BLACK);
      oled_display.drawLine(+i * 8, 5, 5 + (i * 8), 5, WHITE);
    }

#else

    str[i] = (char)219;

    if (!IS_BIT_SET16(*poly_mask, i)) {

      str[i] = (char)'-';
    }
    if (note_interface.is_note(i)) {

      str[i] = (char)255;
    }
#endif
  }
#ifndef OLED_DISPLAY
  GUI.put_string_at(0, str);
#endif
}

void PolyPage::toggle_mask(uint8_t i) {
  if (IS_BIT_SET16(*poly_mask, i)) {
    CLEAR_BIT16(*poly_mask, i);
  } else {
    SET_BIT16(*poly_mask, i);
  }
}

void PolyPage::display() {

  if (!classic_display) {
#ifdef OLED_DISPLAY
    oled_display.clearDisplay();
#endif
  }
  GUI.setLine(GUI.LINE2);
  uint8_t x;
  // GUI.put_string_at(12,"Poly");
  GUI.put_string_at(0, "VOICE SELECT ");

  draw_mask(0);
#ifdef OLED_DISPLAY
  LCD.goLine(1);
  LCD.puts(GUI.lines[1].data);

  if (mcl_cfg.poly_mask != trigled_mask) {
    trigled_mask = mcl_cfg.poly_mask;
    MD.set_trigleds(mcl_cfg.poly_mask, TRIGLED_EXCLUSIVE);
  }

  oled_display.display();
#endif
}

bool PolyPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    uint8_t track = event->source - 128;
    if (midi_active_peering.get_device(event->port)->id != DEVICE_MD) {
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
      note_interface.clear_note(track);
    }
    //  }
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON1) ||
      EVENT_PRESSED(event, Buttons.BUTTON4)) {
    mcl_cfg.write_cfg();
    GUI.ignoreNextEvent(event->source);
    GUI.popPage();
    return true;
  }

  return false;
}

PolyPage poly_page;
