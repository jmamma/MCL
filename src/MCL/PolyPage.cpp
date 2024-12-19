#include "PolyPage.h"
#include "MCLGUI.h"
#include "MD.h"
#include "SeqPages.h"
#include "MidiActivePeering.h"
void PolyPage::setup() {}

void PolyPage::init() {

  poly_mask = &mcl_cfg.poly_mask;
  DEBUG_PRINT_FN();
  trig_interface.on();
  note_interface.init_notes();
  MD.set_trigleds(mcl_cfg.poly_mask, TRIGLED_EXCLUSIVE);
}

void PolyPage::cleanup() {
  seq_ptc_page.init_poly();
  trig_interface.off();
}

void PolyPage::draw_mask() {
  for (uint8_t i = 0; i < 16; i++) {

    uint8_t x = i * 8;

    bool is_note = note_interface.is_note(i);
    oled_display.fillRect(x, 2, 6, 6, is_note);
    if (is_note) {
    }
    else if (IS_BIT_SET16(*poly_mask, i)) {
      oled_display.drawRect(x, 2, 6, 6, WHITE);

    }
    else {
      oled_display.drawLine(x, 5, 5 + x, 5, WHITE);
    }
  }
}

void PolyPage::toggle_mask(uint8_t i) {
  if (IS_BIT_SET16(*poly_mask, i)) {
    CLEAR_BIT16(*poly_mask, i);
  } else {
    SET_BIT16(*poly_mask, i);
  }
}

void PolyPage::display() {
  oled_display.clearDisplay();

  oled_display.setCursor(0, 15);
  oled_display.println("VOICE SELECT ");

  draw_mask();

  if (mcl_cfg.poly_mask != trigled_mask) {
    trigled_mask = mcl_cfg.poly_mask;
    MD.set_trigleds(mcl_cfg.poly_mask, TRIGLED_EXCLUSIVE);
  }

}

bool PolyPage::handleEvent(gui_event_t *event) {
  if (EVENT_CMD(event)) {
    uint8_t key = event->source - 64;
    if (event->mask == EVENT_BUTTON_PRESSED) {
      switch (key) {
      case MDX_KEY_YES:
      case MDX_KEY_NO:
        goto exit;
      }
    }
  }

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
    GUI.ignoreNextEvent(event->source);
  exit:
    mcl_cfg.write_cfg();
    mcl.popPage();
    GUI.currentPage()->init();
    return true;
  }

  return false;
}

PolyPage poly_page;
