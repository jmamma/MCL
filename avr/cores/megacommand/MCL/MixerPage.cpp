#include "MCL.h"
#include "MixerPage.h"

void MixerPage::setup() {
  ((MCLEncoder *)encoders[1])->handler = encoder_level_handle;
  ((MCLEncoder *)encoders[2])->handler = encoder_level_handle;
  create_chars_mixer();
  MD.currentKit = MD.getCurrentKit(CALLBACK_TIMEOUT);
  MD.saveCurrentKit(MD.currentKit);
  MD.getBlockingKit(MD.currentKit);
  DEBUG_PRINTLN("got blocking kit");
  level_pressmode = 0;
  mixer_param1.cur = 60;
  mixer_param2.cur = 60;
#ifdef OLED_DISPLAY
  classic_display = false;
  oled_display.clearDisplay();
#endif
}
void MixerPage::init() {
  md_exploit.on();
  note_interface.state = true;
}
void MixerPage::cleanup() {
  md_exploit.off();
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
#endif
  note_interface.state = false;
}

void MixerPage::set_level(int curtrack, int value) {
  in_sysex = 1;
  MD.setTrackParam(curtrack, 33, value);
  in_sysex = 0;
}
void MixerPage::draw_levels() {
  GUI.setLine(GUI.LINE2);
  uint8_t scaled_level;
  char str[17] = "                ";
  for (int i = 0; i < 16; i++) {
//  if (MD.kit.levels[i] > 120) { scaled_level = 8; }
// else if (MD.kit.levels[i] < 4) { scaled_level = 0; }
#ifdef OLED_DISPLAY

    scaled_level = (uint8_t)(((float)MD.kit.levels[i] / (float)127) * 15);
    oled_display.fillRect(0 + i * 6, 12 + (15 - scaled_level), 4,
                          scaled_level + 1, WHITE);
    /*
    if (scaled_level >= 7) {
      str[i] = (char)0xF8;
    }
    if (scaled_level == 0) {
      str[i] = (char)5;
    } else {
      str[i] = (char)(scaled_level + 6);
    }*/
#else

    scaled_level = (int)(((float)MD.kit.levels[i] / (float)127) * 7);
    if (scaled_level == 7) {
      str[i] = (char)(255);
    } else if (scaled_level > 0) {
      str[i] = (char)(scaled_level + 2);
    }
#endif
  }
  GUI.put_string_at(0, str);
}

void encoder_level_handle(Encoder *enc) {
  MCLEncoder *mdEnc = (MCLEncoder *)enc;

  uint8_t increase = abs(mdEnc->getValue() - mdEnc->old);
  int track_newlevel;
  int dir = 0;
  for (int i = 0; i < 16; i++) {
    if (note_interface.notes[i] == 1) {
      //        set_level(i,mdEnc->getValue() + MD.kit.levels[i] );
      for (int a = 0; a < increase; a++) {
        if ((mdEnc->getValue() - mdEnc->old) < 0) {
          dir = -1;
        }
        //      if ((mdEnc->getValue() - mdEnc->old) > 0) { track_newlevel =
        //      MD.kit.levels[i] + 1; }
        else {
          dir = 1;
        }

        track_newlevel = MD.kit.levels[i] + dir;
        if ((track_newlevel <= 127) && (track_newlevel >= 0)) {
          MD.kit.levels[i] = track_newlevel;

          // if ((MD.kit.levels[i] < 127) && (MD.kit.levels[i] > 0)) {
          mixer_page.set_level(i, MD.kit.levels[i]);
          //}
        }
      }
    }
  }
  mdEnc->cur = 64 + dir;
  mdEnc->old = 64;

  // draw_levels();
}

void MixerPage::display() {
  if (!classic_display) {
    oled_display.clearDisplay();
  }
  note_interface.draw_notes(0);
  if (!classic_display) {
    LCD.goLine(0);
    LCD.puts(GUI.lines[0].data);
  }
  draw_levels();
  if (!classic_display) {
    oled_display.display();
  }
}
bool MixerPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    uint8_t device = midi_active_peering.get_device(port);

    uint8_t track = event->source - 128;

    if (event->mask == EVENT_BUTTON_PRESSED) {
      return true;
    }

    if (event->mask == EVENT_BUTTON_RELEASED) {
      note_interface.draw_notes(0);

      return true;
    }
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER1)) {
    level_pressmode = 1;
    return true;
  }
  if (EVENT_RELEASED(event, Buttons.ENCODER1)) {
    level_pressmode = 0;
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
    GUI.setPage(&cue_page);
    curpage = CUE_PAGE;
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    md_exploit.off();
    GUI.setPage(&grid_page);
    curpage = 0;
    return true;
  }
  return false;
}

