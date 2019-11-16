#include "SeqParamPage.h"
#include "MCL.h"

void SeqParamPage::setup() { SeqPage::setup(); }
void SeqParamPage::config() {
  // config info labels
  const char *str1 = getMachineNameShort(MD.kit.models[last_md_track], 1);
  const char *str2 = getMachineNameShort(MD.kit.models[last_md_track], 2);

  constexpr uint8_t len1 = sizeof(info1);

  char buf[len1] = {'\0'};
  m_strncpy_p(buf, str1, len1);
  strncpy(info1, buf, len1);
  strncat(info1, ">", len1);
  m_strncpy_p(buf, str2, len1);
  strncat(info1, buf, len1);

  strcpy(info2, "PARAM-");
  if (page_id == 0) {
    strcat(info2, "A");
  } else {
    strcat(info2, "B");
  }
}

void SeqParamPage::init() {
  config();
  md_exploit.on();
  note_interface.state = true;

  ((MCLEncoder *)encoders[0])->max = 24;
  ((MCLEncoder *)encoders[1])->max = 128;
  ((MCLEncoder *)encoders[2])->max = 24;
  ((MCLEncoder *)encoders[3])->max = 128;

  ((MCLEncoder *)encoders[2])->handler = NULL;

  encoders[0]->cur = mcl_seq.md_tracks[last_md_track].locks_params[p1];
  encoders[2]->cur = mcl_seq.md_tracks[last_md_track].locks_params[p2];
  encoders[1]->cur =
      MD.kit.params[last_md_track]
                   [mcl_seq.md_tracks[last_md_track].locks_params[p1]];
  encoders[3]->cur =
      MD.kit.params[last_md_track]
                   [mcl_seq.md_tracks[last_md_track].locks_params[p2]];
  // Prevent hasChanged from being called
  encoders[0]->old = encoders[0]->cur;
  encoders[1]->old = encoders[1]->cur;
  encoders[2]->old = encoders[2]->cur;
  encoders[3]->old = encoders[3]->cur;

  midi_events.setup_callbacks();
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
#endif
}

void SeqParamPage::construct(uint8_t p1_, uint8_t p2_) {
  p1 = p1_;
  p2 = p2_;
}

void SeqParamPage::cleanup() {
  SeqPage::cleanup();
  midi_events.remove_callbacks();
}

#ifndef OLED_DISPLAY
void SeqParamPage::display() {
  GUI.setLine(GUI.LINE1);
  char myName[4] = "-- ";
  char myName2[4] = "-- ";
  if (encoders[0]->getValue() == 0) {
    GUI.put_string_at(0, "--");
  } else {
    PGM_P modelname = NULL;
    modelname = model_param_name(MD.kit.models[last_md_track],
                                 encoders[0]->getValue() - 1);
    if (modelname != NULL) {
      m_strncpy_p(myName, modelname, 4);
    }
    GUI.put_string_at(0, myName);
  }
  if (encoders[1]->getValue() == 0) {
    GUI.put_string_at(4, "--");
  } else {
    GUI.put_value_at2(4, encoders[1]->getValue() - 1);
  }
  if (encoders[2]->getValue() == 0) {
    GUI.put_string_at(7, "--");
  } else {
    PGM_P modelname = NULL;
    modelname = model_param_name(MD.kit.models[last_md_track],
                                 encoders[2]->getValue() - 1);
    if (modelname != NULL) {
      m_strncpy_p(myName2, modelname, 4);
    }
    GUI.put_string_at(7, myName2);
  }
  if (encoders[3]->getValue() == 0) {
    GUI.put_string_at(11, "--");
  } else {
    GUI.put_value_at2(11, encoders[3]->getValue() - 1);
  }
  if (page_id == 0) {
    GUI.put_string_at(14, "A");
  }
  if (page_id == 1) {
    GUI.put_string_at(14, "B");
  }
  GUI.put_value_at1(15, (page_select + 1));
  draw_lock_mask(page_select * 16);
  SeqPage::display();
}
#else
void SeqParamPage::display() {
  SeqPage::display();
  draw_knob_frame();

  char myName[4] = "-- ";
  char myName2[4] = "-- ";

  if (encoders[0]->getValue() != 0) {
    PGM_P modelname = NULL;
    modelname = model_param_name(MD.kit.models[last_md_track],
                                 encoders[0]->getValue() - 1);
    if (modelname != NULL) {
      m_strncpy_p(myName, modelname, 4);
    }
  }

  if (encoders[2]->getValue() != 0) {
    PGM_P modelname = NULL;
    modelname = model_param_name(MD.kit.models[last_md_track],
                                 encoders[2]->getValue() - 1);
    if (modelname != NULL) {
      m_strncpy_p(myName2, modelname, 4);
    }
  }

  draw_knob(0, "TGT", myName);
  draw_knob(2, "TGT", myName2);

  draw_knob(1, encoders[1], "VAL");
  draw_knob(3, encoders[3], "VAL");
  draw_pattern_mask(page_select * 16, DEVICE_MD);
  draw_lock_mask(page_select * 16);

  oled_display.display();
}

#endif
void SeqParamPage::loop() {

  if (encoders[0]->hasChanged() || encoders[1]->hasChanged() ||
      encoders[2]->hasChanged() || encoders[3]->hasChanged()) {
    for (uint8_t n = 0; n < 16; n++) {

      if (note_interface.notes[n] == 1) {
        uint8_t step = n + (page_select * 16);
        int8_t utiming = mcl_seq.md_tracks[last_md_track].timing[step]; // upper
        uint8_t condition =
            mcl_seq.md_tracks[last_md_track].conditional[step]; // lower

        // Fudge timing info if it's not there
        if (utiming == 0) {
          utiming = 12;
          mcl_seq.md_tracks[last_md_track].conditional[step] = condition;
          mcl_seq.md_tracks[last_md_track].timing[step] = utiming;
        }
        SET_BIT64(mcl_seq.md_tracks[last_md_track].lock_mask, step);

        mcl_seq.md_tracks[last_md_track].locks[p1][step] = encoders[1]->cur;
        mcl_seq.md_tracks[last_md_track].locks[p2][step] = encoders[3]->cur;
      }
    }
    if (encoders[0]->hasChanged() || encoders[2]->hasChanged()) {
      mcl_seq.md_tracks[last_md_track].reset_params();
      mcl_seq.md_tracks[last_md_track].locks_params[p1] = encoders[0]->cur;
      mcl_seq.md_tracks[last_md_track].locks_params[p2] = encoders[2]->cur;
      mcl_seq.md_tracks[last_md_track].update_params();
    }
  }
}
bool SeqParamPage::handleEvent(gui_event_t *event) {

  if (SeqPage::handleEvent(event)) {
    return true;
  }

  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    uint8_t device = midi_active_peering.get_device(port);

    uint8_t track = event->source - 128;

    if (device == DEVICE_A4) {
      return true;
    }
    uint8_t step = track + (page_select * 16);

    if (event->mask == EVENT_BUTTON_PRESSED) {
      uint8_t param_offset;
      encoders[0]->cur = mcl_seq.md_tracks[last_md_track].locks_params[p1];
      encoders[2]->cur = mcl_seq.md_tracks[last_md_track].locks_params[p2];

      encoders[1]->cur = mcl_seq.md_tracks[last_md_track].locks[p1][step];
      encoders[3]->cur = mcl_seq.md_tracks[last_md_track].locks[p2][step];
    }
    if (event->mask == EVENT_BUTTON_RELEASED) {
      if (device == DEVICE_A4) {
        // GUI.setPage(&seq_extstep_page);
        return true;
      }

      /*      int8_t utiming = mcl_seq.md_tracks[last_md_track].timing[step]; //
upper uint8_t condition = mcl_seq.md_tracks[last_md_track].conditional[step]; //
lower

// Fudge timing info if it's not there
if (utiming == 0) {
  utiming = 12;
  mcl_seq.md_tracks[last_md_track].conditional[step] = condition;
  mcl_seq.md_tracks[last_md_track].timing[step] = utiming;
}*/
      if (IS_BIT_SET64(mcl_seq.md_tracks[last_md_track].lock_mask, step)) {
        if (clock_diff(note_interface.note_hold, slowclock) < TRIG_HOLD_TIME) {
          CLEAR_BIT64(mcl_seq.md_tracks[last_md_track].lock_mask, step);
        }
      } else {
        SET_BIT64(mcl_seq.md_tracks[last_md_track].lock_mask, step);
      }
      /*
            mcl_seq.md_tracks[last_md_track].locks[p1][step] =
                encoders[1]->cur;
            mcl_seq.md_tracks[last_md_track].locks[p2][step] =
                encoders[3]->cur;

            mcl_seq.md_tracks[last_md_track].locks_params[p1] =
         encoders[0]->cur; mcl_seq.md_tracks[last_md_track].locks_params[p2] =
         encoders[2]->cur;
        */
    }
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER3)) {
    if (note_interface.notes_all_off() || (note_interface.notes_count() == 0)) {
      GUI.setPage(&grid_page);
    }
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
    mcl_seq.md_tracks[last_md_track].clear_locks();
    return true;
  }

  if ((EVENT_PRESSED(event, Buttons.BUTTON1) && BUTTON_DOWN(Buttons.BUTTON4)) ||
      (EVENT_PRESSED(event, Buttons.BUTTON4) && BUTTON_DOWN(Buttons.BUTTON3))) {

    for (uint8_t n = 0; n < 16; n++) {
      mcl_seq.md_tracks[n].clear_locks();
    }
    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    uint8_t page_depth = page_id;
    if (page_depth < NUM_PARAM_PAGES - 1) {
      page_depth = page_depth + 1;
    } else {
      page_depth = 0;
    }

    GUI.setPage(&seq_param_page[page_depth]);
    return true;
  }
  if (EVENT_RELEASED(event, Buttons.BUTTON4)) {
    mcl_seq.md_tracks[last_md_track].clear_locks();
    return true;
  }

  return false;
}

void SeqParamPageMidiEvents::onNoteOffCallback_Midi2(uint8_t *msg) {}

void SeqParamPageMidiEvents::onNoteOnCallback_Midi2(uint8_t *msg) {}
void SeqParamPageMidiEvents::setup_callbacks() {
  if (state) {
    return;
  }

  Midi2.addOnNoteOnCallback(
      this,
      (midi_callback_ptr_t)&SeqParamPageMidiEvents::onNoteOnCallback_Midi2);
  Midi2.addOnNoteOffCallback(
      this,
      (midi_callback_ptr_t)&SeqParamPageMidiEvents::onNoteOffCallback_Midi2);

  state = true;
}

void SeqParamPageMidiEvents::remove_callbacks() {

  if (!state) {
    return;
  }

  Midi2.removeOnNoteOnCallback(
      this,
      (midi_callback_ptr_t)&SeqParamPageMidiEvents::onNoteOnCallback_Midi2);
  Midi2.removeOnNoteOffCallback(
      this,
      (midi_callback_ptr_t)&SeqParamPageMidiEvents::onNoteOffCallback_Midi2);

  state = false;
}
