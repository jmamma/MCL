#include "MCL.h"
#include "SeqStepPage.h"

#define MIDI_OMNI_MODE 17

void SeqStepPage::setup() { SeqPage::setup(); }
void SeqStepPage::init() {
  DEBUG_PRINT_FN();
  DEBUG_PRINTLN("init seqstep");
  SeqPage::init();
  ((MCLEncoder *)encoders[0])->max = 13;
  ((MCLEncoder *)encoders[1])->max = 23;
  ((MCLEncoder *)encoders[1])->min = 1;
  encoders[1]->cur = 12;
  encoders[0]->cur = 0;
  ((MCLEncoder *)encoders[2])->max = 64;
  encoders[2]->cur = mcl_seq.md_tracks[last_md_track].length;

  tuning_t const *tuning = MD.getModelTuning(MD.kit.models[last_md_track]);

  ((MCLEncoder *)encoders[3])->max = tuning->len - 1;
  midi_events.setup_callbacks();
  curpage = SEQ_STEP_PAGE;
  md_exploit.on();
  note_interface.state = true;
}
void SeqStepPage::cleanup() {
  midi_events.remove_callbacks();
  SeqPage::cleanup();
  if (MidiClock.state != 2) {
    MD.setTrackParam(last_md_track, 0,
                       MD.kit.params[last_md_track][0]);
  }
}
void SeqStepPage::display() {
  GUI.setLine(GUI.LINE1);
  GUI.put_string_at(0, "                ");

  const char *str1 = getMachineNameShort(MD.kit.models[last_md_track], 1);
  const char *str2 = getMachineNameShort(MD.kit.models[last_md_track], 2);

  char c[3] = "--";

  if (encoders[0]->getValue() == 0) {
    GUI.put_string_at(0, "L1");

  } else if (encoders[0]->getValue() <= 8) {
    GUI.put_string_at(0, "L");

    GUI.put_value_at1(1, encoders[0]->getValue());

  } else {
    GUI.put_string_at(0, "P");
    uint8_t prob[5] = {1, 2, 5, 7, 9};
    GUI.put_value_at1(1, prob[encoders[0]->getValue() - 9]);
  }

  if (encoders[1]->getValue() == 0) {
    GUI.put_string_at(2, "--");
  } else if ((encoders[1]->getValue() < 12) && (encoders[1]->getValue() != 0)) {
    GUI.put_string_at(2, "-");
    GUI.put_value_at2(3, 12 - encoders[1]->getValue());

  } else {
    GUI.put_string_at(2, "+");
    GUI.put_value_at2(3, encoders[1]->getValue() - 12);
  }

  if (show_pitch) {
    tuning_t const *tuning = MD.getModelTuning(MD.kit.models[last_md_track]);
    if (tuning != NULL) {
      if (encoders[3]->cur == 0) {
        GUI.put_string_at(10, "--");
      } else {
        uint8_t base = tuning->base;
        uint8_t notenum = encoders[3]->cur + base;
        MusicalNotes number_to_note;
        uint8_t oct = notenum / 12;
        uint8_t note = notenum - 12 * (notenum / 12);
        GUI.put_string_at(10, number_to_note.notes_upper[note]);
        GUI.put_value_at1(12, oct);
      }
    }
  } else {
    GUI.put_p_string_at(10, str1);
    GUI.put_p_string_at(12, str2);
  }
  GUI.put_value_at(6, encoders[2]->getValue());
  GUI.put_value_at1(15, page_select + 1);
  draw_pattern_mask((page_select * 16), DEVICE_MD);
}
void SeqStepPage::loop() {
  if (encoders[0]->hasChanged() || encoders[1]->hasChanged() ||
      encoders[3]->hasChanged()) {
    tuning_t const *tuning = MD.getModelTuning(MD.kit.models[last_md_track]);

    for (uint8_t n = 0; n < 16; n++) {

      if (note_interface.notes[n] == 1) {
        uint8_t step = n + (page_select * 16);
        if (step < mcl_seq.md_tracks[last_md_track].length) {

          uint8_t utiming = (encoders[1]->cur + 0);
          uint8_t condition = encoders[0]->cur;

          //  timing = 3;
          // condition = 3;
          mcl_seq.md_tracks[last_md_track].conditional[step] = condition;
          mcl_seq.md_tracks[last_md_track].timing[step] = utiming; // upper

      if (!IS_BIT_SET64(mcl_seq.md_tracks[last_md_track].pattern_mask, step)) {

        SET_BIT64(mcl_seq.md_tracks[last_md_track].pattern_mask, step);
       }
          if ((encoders[3]->cur > 0) && (last_md_track < 15) &&
              (tuning != NULL)) {
            uint8_t base = tuning->base;
            uint8_t note_num = encoders[3]->cur;
            uint8_t machine_pitch = pgm_read_byte(&tuning->tuning[note_num]);
            mcl_seq.md_tracks[last_md_track].set_track_pitch(step,
                                                             machine_pitch);
          }
        }
      }
    }
  }
}
bool SeqStepPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    uint8_t device = midi_active_peering.get_device(port);

    uint8_t track = event->source - 128;

    midi_device = device;
    if (event->mask == EVENT_BUTTON_PRESSED) {
      if (device == DEVICE_A4) {
        // GUI.setPage(&seq_extstep_page);
        return true;
      }
      // track 16 should not have locks
      if (last_md_track < 15) {
        show_pitch = true;
      }
      if ((track + (page_select * 16)) >=
          mcl_seq.md_tracks[last_md_track].length) {
        return true;
      }

      ((MCLEncoder *)encoders[1])->max = 23;
      uint8_t step = track + (page_select * 16);
      int8_t utiming = mcl_seq.md_tracks[last_md_track].timing[step]; // upper
      uint8_t condition =
          mcl_seq.md_tracks[last_md_track].conditional[step]; // lower
      uint8_t pitch =
          mcl_seq.md_tracks[last_md_track].get_track_lock(step, 0) - 1;
      // Cond
      encoders[0]->cur = condition;
      uint8_t note_num = 255;

      // SET_BIT64(mcl_seq.md_tracks[last_md_track].pattern_mask, step);
      tuning_t const *tuning = MD.getModelTuning(MD.kit.models[last_md_track]);
      if (tuning) {
        for (uint8_t i = 0; i < tuning->len && note_num == 255; i++) {
          uint8_t ccStored = pgm_read_byte(&tuning->tuning[i]);
          if (ccStored >= pitch) {
            note_num = i;
          }
        }
        if (note_num == 255) {
          encoders[3]->cur = 0;
        } else {
          encoders[3]->cur = note_num;
        }
      }
      // Micro
      if (utiming == 0) {
        utiming = 12;
      }
      encoders[1]->cur = utiming;
    }
    if (event->mask == EVENT_BUTTON_RELEASED) {

      if (device == DEVICE_A4) {
        // GUI.setPage(&seq_extstep_page);
        return true;
      }

      if (last_md_track < 15) {
        show_pitch = false;
      }
      if ((track + (page_select * 16)) >=
          mcl_seq.md_tracks[last_md_track].length) {
        return true;
      }

      uint8_t step = track + (page_select * 16);
      /*      uint8_t utiming = (encoders[1]->cur + 0);
            uint8_t condition = encoders[0]->cur;

            uint8_t step = track + (page_select * 16);
            //  timing = 3;
            // condition = 3;
            mcl_seq.md_tracks[last_md_track].conditional[step] = condition;
            mcl_seq.md_tracks[last_md_track].timing[step] = utiming; // upper
            if ((encoders[3]->cur > 0) && (last_md_track < 15)) {
              tuning_t const *tuning =
                  MD.getModelTuning(MD.kit.models[last_md_track]);
              if (tuning != NULL) {
                uint8_t base = tuning->base;
                uint8_t note_num = encoders[3]->cur;
                uint8_t machine_pitch =
         pgm_read_byte(&tuning->tuning[note_num]);
                mcl_seq.md_tracks[last_md_track].set_track_pitch(step,
         machine_pitch);
              }
            }*/
      //   conditional_timing[cur_col][(track + (encoders[1]->cur * 16))] =
      //   condition; //lower

      if (!IS_BIT_SET64(mcl_seq.md_tracks[last_md_track].pattern_mask, step)) {
        uint8_t utiming = (encoders[1]->cur + 0);
        uint8_t condition = encoders[0]->cur;

        mcl_seq.md_tracks[last_md_track].conditional[step] = condition;
        mcl_seq.md_tracks[last_md_track].timing[step] = utiming; // upper
        mcl_seq.md_tracks[last_md_track].clear_step_locks(step);
        SET_BIT64(mcl_seq.md_tracks[last_md_track].pattern_mask, step);
      } else {
        DEBUG_PRINTLN("Trying to clear");
        if ((slowclock - note_interface.note_hold) < TRIG_HOLD_TIME) {
          CLEAR_BIT64(mcl_seq.md_tracks[last_md_track].pattern_mask, step);
          mcl_seq.md_tracks[last_md_track].conditional[step] = 0;
          mcl_seq.md_tracks[last_md_track].timing[step] = 12; // upper
        }
      }
      // Cond
      // encoders[3]->cur = condition;
      // Microƒ
      // encoders[4]->cur = timing;
      // draw_notes(1);
      return true;
    }
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER1)) {
    GUI.setPage(&grid_page);
    return true;
  }

  if ((EVENT_PRESSED(event, Buttons.BUTTON1) && BUTTON_DOWN(Buttons.BUTTON4)) ||
      (EVENT_PRESSED(event, Buttons.BUTTON4) && BUTTON_DOWN(Buttons.BUTTON3))) {
    for (uint8_t n = 0; n < 16; n++) {
      mcl_seq.md_tracks[n].clear_track();
    }
    return true;
  }
  if (EVENT_RELEASED(event, Buttons.BUTTON4)) {
    mcl_seq.md_tracks[last_md_track].clear_track();
    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    GUI.setPage(&seq_extstep_page);
    return true;
    /*

      return true;
    */
  }
  if (SeqPage::handleEvent(event)) {
    return true;
  }

  return false;
}

void SeqStepMidiEvents::onNoteOnCallback_Midi2(uint8_t *msg) {

  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);

  tuning_t const *tuning = MD.getModelTuning(MD.kit.models[last_md_track]);
  uint8_t note_num = msg[1] - tuning->base;
  if (note_num < tuning->len) {
    uint8_t machine_pitch = pgm_read_byte(&tuning->tuning[note_num]);
    if (MidiClock.state != 2) {
      MD.setTrackParam(last_md_track, 0, machine_pitch);
      MD.triggerTrack(last_md_track, 127);
    }
    seq_step_page.encoders[3]->cur = note_num;
  }
}

void SeqStepMidiEvents::setup_callbacks() {
  if (state) {
    return;
  }
  Midi2.addOnNoteOnCallback(
      this, (midi_callback_ptr_t)&SeqStepMidiEvents::onNoteOnCallback_Midi2);

  state = true;
}

void SeqStepMidiEvents::remove_callbacks() {

  if (!state) {
    return;
  }
  Midi2.removeOnNoteOnCallback(
      this, (midi_callback_ptr_t)&SeqStepMidiEvents::onNoteOnCallback_Midi2);
  state = false;
}
