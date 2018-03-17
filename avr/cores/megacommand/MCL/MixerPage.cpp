#include "MixerPage.h"

void MixerPage::set_level(int curtrack, int value) {
  uint8_t cc;
  uint8_t channel = curtrack >> 2;
  if (curtrack < 4) {
    cc = 8 + curtrack;
  } else if (curtrack < 8) {
    cc = 4 + curtrack;
  } else if (curtrack < 12) {
    cc = curtrack;
  } else if (curtrack < 16) {
    cc = curtrack - 4;
  }
  USE_LOCK();
  SET_LOCK();
  if (md_exploit.state) {
    MidiUart.sendCC(channel + 3, cc, value);
  } else {
    MidiUart.sendCC(channel + 9, cc, value);
  }
  CLEAR_LOCK();
  in_sysex = 0;
}
void MixerPage::draw_levels() {
  GUI.setLine(GUI.LINE2);
  uint8_t scaled_level;
  char str[17] = "                ";
  for (int i = 0; i < 16; i++) {
    //  if (MD.kit.levels[i] > 120) { scaled_level = 8; }
    // else if (MD.kit.levels[i] < 4) { scaled_level = 0; }
    scaled_level = (int)(((float)MD.kit.levels[i] / (float)127) * 7);
    if (scaled_level == 7) {
      str[i] = (char)(255);
    } else if (scaled_level > 0) {
      str[i] = (char)(scaled_level + 2);
    }
  }
  GUI.put_string_at(0, str);
}

void MixerPage::encoder_level_handle(Encoder *enc) {
  TrackInfoEncoder *mdEnc = (TrackInfoEncoder *)enc;
  uint8_t increase = 0;
  if (enc->pressmode == false) {
    increase = 1;
  }
  if (enc->pressmode == true) {
    increase = 4;
  }
  int track_newlevel;
  for (int i = 0; i < 16; i++) {
    if (notes[i] == 1) {
      //        setLevel(i,mdEnc->getValue() + MD.kit.levels[i] );
      for (int a = 0; a < increase; a++) {
        if ((mdEnc->getValue() - mdEnc->old) < 0) {
          track_newlevel = MD.kit.levels[i] - 1;
        }
        //      if ((mdEnc->getValue() - mdEnc->old) > 0) { track_newlevel =
        //      MD.kit.levels[i] + 1; }
        else {
          track_newlevel = MD.kit.levels[i] + 1;
        }
        if ((track_newlevel <= 127) && (track_newlevel >= 0)) {
          MD.kit.levels[i] += mdEnc->getValue() - mdEnc->old;
          // if ((MD.kit.levels[i] < 127) && (MD.kit.levels[i] > 0)) {
          setLevel(i, MD.kit.levels[i]);
          //}
        }
      }
    }
  }
  if (mdEnc->getValue() >= 127) {
    mdEnc->cur = 1;
    mdEnc->old = 0;
  } else if (mdEnc->getValue() <= 0) {
    mdEnc->cur = 126;
    mdEnc->old = 127;
  }

  // draw_levels();
}

void MixerPage::display() {
  note_interface.draw_notes(0);
  this.draw_levels();
}
bool MixerPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t device = midi_active_peering.get_device(port);

    uint8_t track = event->source - 128;

    if (event->mask == EVENT_BUTTON_PRESSED) {
      return true;
    }

    if (event->mask == EVENT_BUTTON_RELEASED) {
              draw_notes(0);

      return true;
    }
  }
  if (EVENT_PRESSED(evt, Buttons.ENCODER1)) {
    level_pressmode = 1;
    return true;
  }
  if (EVENT_RELEASED(evt, Buttons.ENCODER1)) {
    level_pressmode = 0;
    return true;
  }

  if (EVENT_PRESSED(evt, Buttons.BUTTON1)) {

    curpage = CUE_PAGE;
    return true;
  }
  if (EVENT_PRESSED(evt, Buttons.BUTTON2)) {
    md_exploit.off();
    GUI.setPage(&grid_page);
    curpage = 0;
    return true;
  }
  return false;
}

void MixerPage::create_chars_mixer() {
  uint8_t temp_charmap[8] = {0, 0, 0, 0, 0, 0, 0, 31};

  for (uint8_t i = 1; i < 8; i++) {
    for (uint8_t x = 1; x < i; x++) {
      temp_charmap[(8 - x)] = 31;
      LCD.createChar(1 + i, temp_charmap);
    }
  }
}
bool MixerPage::setup() {
  Encoders[1]->handler = encoder_level_handle;
  Encoders[2]->handler = encoder_level_handle;
  create_chars_mixer();
  currentkit_temp = MD.getCurrentKit(CALLBACK_TIMEOUT);
  curpage = MIXER_PAGE;
  MD.saveCurrentKit(currentkit_temp);
  MD.getBlockingKit(currentkit_temp);
  level_pressmode = 0;
  mixer_param1.cur = 60;
  md_exploit.on();
  note_inteface.collect_trigs = true;
}
