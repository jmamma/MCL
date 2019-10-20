#include "FXPage.h"
#include "MCL.h"
#include "RAMPage.h"
#define FX_TYPE 0
#define FX_PARAM 1
#define NUM_OF_ENCODERS 4

void FXPage::setup() { DEBUG_PRINT_FN(); }

void FXPage::init() {
  DEBUG_PRINT_FN();
#ifdef OLED_DISPLAY
  classic_display = false;
  oled_display.clearDisplay();
  oled_display.setFont();
#endif
  md_exploit.off();
      for (uint8_t a = 0; a < num_of_params; a++) {
      DEBUG_PRINTLN("params");
      DEBUG_PRINTLN(params[a].type);
      DEBUG_PRINTLN(params[a].param);
      } 


  for (uint8_t n = 0; n < num_of_params; n++) {

    uint8_t fx_param = params[n].param;
    switch (params[n].type) {
    case MD_FX_ECHO:
      DEBUG_PRINTLN("setting delay");
      DEBUG_PRINTLN(n);
      DEBUG_PRINTLN(fx_param); 
      encoders[n]->cur = MD.kit.delay[fx_param];
      break;
    case MD_FX_REV:
      encoders[n]->cur = MD.kit.reverb[fx_param];
      break;
    case MD_FX_EQ:
      encoders[n]->cur = MD.kit.eq[fx_param];
      break;
    case MD_FX_DYN:
      encoders[n]->cur = MD.kit.dynamics[fx_param];
      break;
    }

    encoders[n]->old = encoders[n]->cur;
  }
}
void FXPage::cleanup() {
  //  md_exploit.off();
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
#endif
}

void FXPage::loop() {

  for (uint8_t i = 0; i < NUM_OF_ENCODERS; i++) {
    uint8_t n = i + ((page_mode ?  1 : 0) * NUM_OF_ENCODERS);

    if (encoders[i]->hasChanged()) {
        uint8_t fx_param = params[n].param;
        uint8_t fx_type = params[n].type;

      uint8_t val;
      //Interpolation.
      for (val = encoders[i]->old; val < encoders[i]->cur; val++) {
        MD.sendFXParam(fx_param, val, fx_type);
      }
      for (val = encoders[i]->old; val > encoders[i]->cur; val--) {
        MD.sendFXParam(fx_param, val, fx_type);
      }

    }
  }
}
void FXPage::display() {

  if (!classic_display) {
#ifdef OLED_DISPLAY
    oled_display.clearDisplay();
#endif
  }
#ifndef OLED_DISPLAY
  GUI.clearLines();
  GUI.setLine(GUI.LINE1);
  uint8_t x;

  GUI.put_string_at(0, "FX");
  GUI.put_value_at1(4, page_mode ? 1 : 0);
  GUI.setLine(GUI.LINE2);
  /*
    if (mcl_cfg.ram_page_mode == 0) {
      GUI.put_string_at(0, "MON");
    } else {
      GUI.put_string_at(0, "LNK");
    }
  */

#endif
#ifdef OLED_DISPLAY
  oled_display.setFont();
  oled_display.setCursor(0, 0);

  oled_display.print("FX ");
  oled_display.print(page_mode ? 1 : 0);
  oled_display.print(" ");
   for (uint8_t i = 0; i < NUM_OF_ENCODERS; i++) {
    uint8_t n = i + ((page_mode ?  1 : 0) * NUM_OF_ENCODERS);

        uint8_t fx_param = params[n].param;
        uint8_t fx_type = params[n].type;

   
    mcl_gui.draw_encoder(10 + 20 * i, 10, encoders[i]->cur);
   }
 
   mcl_gui.draw_encoder(100,10,0);
   oled_display.display();

#endif

}
void FXPage::onControlChangeCallback_Midi(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
  uint8_t track;
  uint8_t track_param;
  // If external keyboard controlling MD pitch, send parameter updates
  // to all polyphonic tracks
  uint8_t param_true = 0;

  MD.parseCC(channel, param, &track, &track_param);
}

void FXPage::setup_callbacks() {
  if (midi_state) {
    return;
  }
  Midi.addOnControlChangeCallback(
      this, (midi_callback_ptr_t)&FXPage::onControlChangeCallback_Midi);

  midi_state = true;
}

void FXPage::remove_callbacks() {
  if (!midi_state) {
    return;
  }

  Midi.removeOnControlChangeCallback(
      this, (midi_callback_ptr_t)&FXPage::onControlChangeCallback_Midi);

  midi_state = false;
}

bool FXPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    uint8_t track = event->source - 128;
    if (midi_active_peering.get_device(event->port) != DEVICE_MD) {
      return true;
    }
  }
  if (event->mask == EVENT_BUTTON_RELEASED) {
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER1) ||
      EVENT_PRESSED(event, Buttons.ENCODER2) ||
      EVENT_PRESSED(event, Buttons.ENCODER3) ||
      EVENT_PRESSED(event, Buttons.ENCODER4)) {
    GUI.setPage(&grid_page);
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
  page_mode = !(page_mode);
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
  }

  return false;
}
