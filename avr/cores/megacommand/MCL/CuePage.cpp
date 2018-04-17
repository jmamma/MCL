#include "CuePage.h"
#include "MCL.h"

void CuePage::setup() { md_exploit.on(); }
void CuePage::init() {
  md_exploit.on();
  note_interface.state = true;
}
void CuePage::cleanup() {
  md_exploit.off();
  note_interface.state = false;
}
void CuePage::set_level(int curtrack, int value) {
  MD.setTrackParam(curtrack, 33, value);
}

void CuePage::toggle_cue(int i) {
  if (IS_BIT_SET32(mcl_cfg.cues, i)) {
    CLEAR_BIT32(mcl_cfg.cues, i);
    MD.setTrackRouting(i, 6);
  } else {
    SET_BIT32(mcl_cfg.cues, i);
    MD.setTrackRouting(i, 5);
  }
}
void CuePage::toggle_cues_batch() {
  uint16_t quantize_mute;
  quantize_mute = 1 << encoders[2]->getValue();
  int i;
  for (i = 0; i < 16; i++) {
    if (note_interface.notes[i] == 3) {
      MD.muteTrack(i, true);
    }
  }
  if (encoders[2]->getValue() < 7) {
    while (
        (((MidiClock.div32th_counter - mcl_actions_callbacks.start_clock32th) +
          3) %
         (quantize_mute * 2)) != 0) {
      GUI.display();
    }
  }

  // send the track to master before unmuting

  for (i = 0; i < 16; i++) {
    if (note_interface.notes[i] == 3) {
      if (encoders[2]->getValue() == 7) {
        set_level(i, 0);
      }
      toggle_cue(i);

      MD.muteTrack(i, false);
    }
    //  note_interface.notes[i] = 0;
    // trackinfo_page.display();
  }
}
void CuePage::display() {
  GUI.setLine(GUI.LINE2);
  uint8_t x;
  // GUI.put_string_at(12,"Cue");
  GUI.put_string_at(0, "CUES     ");

  GUI.put_string_at(9, "Q:");
  if (encoders[2]->getValue() == 0) {
    GUI.put_string_at(11, "--");
  } else if (encoders[2]->getValue() == 7) {
    GUI.put_string_at(11, "LV");
  } else {
    x = 1 << encoders[2]->getValue();

    GUI.put_value_at2(11, x);
  }
  uint8_t step_count =
      (MidiClock.div16th_counter - mcl_actions_callbacks.start_clock32th / 2) -
      (64 * ((MidiClock.div16th_counter -
              mcl_actions_callbacks.start_clock32th / 2) /
             64));
  GUI.put_value_at2(14, step_count);
  note_interface.draw_notes(0);
}
bool CuePage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    uint8_t track = event->source - 128;
    if (midi_active_peering.get_device(event->port) != DEVICE_MD) {
      return true;
    }
    note_interface.draw_notes(0);
    if (event->mask == EVENT_BUTTON_PRESSED) {

      if ((encoders[2]->getValue() == 0)) {
        toggle_cue(track);
        md_exploit.send_globals();
      }
    }
    if (event->mask == EVENT_BUTTON_RELEASED) {
      if ((encoders[2]->getValue() == 0)) {
        note_interface.notes[track] = 0;
      }

      if (note_interface.notes_all_off()) {
        if ((encoders[2]->getValue() > 0) &&
            (note_interface.notes_count_off() > 1)) {
          toggle_cues_batch();
          md_exploit.send_globals();
          md_exploit.off();
          GUI.setPage(&grid_page);
          curpage = 0;
        }
      }
    }
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
    GUI.setPage(&mixer_page);
    return true;
  }
  return false;
}
