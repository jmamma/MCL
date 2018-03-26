#include "MCL.h"
#include "SeqExtStepPage.h"

void SeqExtStepPage::setup() { SeqPage::setup(); }

void SeqExtStepPage::init() {
  if (mcl_seq.ext_tracks[last_ext_track].resolution == 1) {
    ((MCLEncoder *)encoders[1])->cur = 6;
    ((MCLEncoder *)encoders[1])->max = 11;
  } else {
    ((MCLEncoder *)encoders[1])->cur = 12;
    ((MCLEncoder *)encoders[1])->max = 23;
  }
  encoders[2]->cur = mcl_seq.ext_tracks[last_ext_track].length;
  grid_page.cur_col = last_ext_track + 16;
  curpage = SEQ_EXTSTEP_PAGE;
  midi_events.setup_callbacks();
}

void SeqExtStepPage::cleanup() { midi_events.remove_callbacks(); }

struct musical_notes {
  const char *notes_upper[16] = {"C ", "C#", "D ", "D#", "E ", "F",
                                 "F#", "G ", "G#", "A ", "A#", "B "};
  const char *notes_lower[16] = {"c ", "c#", "d ", "d#", "e ", "f",
                                 "f#", "g ", "g#", "a ", "a#", "b "};
};

void SeqExtStepPage::pattern_len_handler(Encoder *enc) {

  if (BUTTON_DOWN(Buttons.BUTTON3)) {
    for (uint8_t c = 0; c < mcl_seq.num_ext_tracks; c++) {
      mcl_seq.ext_tracks[c].length = encoders[2]->getValue();
    }
  }
}

void SeqExtStepPage::display() {
  GUI.put_string_at(0, "                ");

  const char *str1 =
      getMachineNameShort(MD.kit.models[last_md_track], 1);
  const char *str2 =
      getMachineNameShort(MD.kit.models[last_md_track], 2);

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

  // Cond
  //    GUI.put_value_at2(0, encoders[1]->getValue());
  // Pos
  // 0  1   2  3  4  5  6  7  8  9  10  11
  //  -5  -4 -3 -2 -1 0
  if (mcl_seq.ext_tracks[last_ext_track].resolution == 1) {
    if (encoders[1]->getValue() == 0) {
      GUI.put_string_at(2, "--");
    } else if ((encoders[1]->getValue() < 6) &&
               (encoders[1]->getValue() != 0)) {
      GUI.put_string_at(2, "-");
      GUI.put_value_at1(3, encoders[1]->getValue() - 6);
    } else {
      GUI.put_string_at(2, "+");
      GUI.put_value_at1(3, encoders[1]->getValue() - 6);
    }
  } else {
    if (encoders[1]->getValue() == 0) {
      GUI.put_string_at(2, "--");
    } else if ((encoders[1]->getValue() < 12) &&
               (encoders[1]->getValue() != 0)) {
      GUI.put_string_at(2, "-");
      GUI.put_value_at1(3, 12 - encoders[1]->getValue());

    } else {
      GUI.put_string_at(2, "+");
      GUI.put_value_at1(3, encoders[1]->getValue() - 12);
    }
  }

  struct musical_notes number_to_note;
  uint8_t notenum;
  uint8_t notes_held = 0;
  uint8_t i;
  for (i = 0; i < 16; i++) {
    if (note_interface.notes[i] == 1) {
      notes_held += 1;
    }
  }

  if (notes_held > 0) {
    for (i = 0; i < 4; i++) {

      notenum = mcl_seq.ext_tracks[last_ext_track]
                    .notes[i][note_interface.last_note + page_select * 16];
      if (notenum != 0) {
        notenum = notenum - 1;
        uint8_t oct = notenum / 12;
        uint8_t note = notenum - 12 * (notenum / 12);
        if (mcl_seq.ext_tracks[last_ext_track]
                .notes[i][note_interface.last_note + page_select * 16] > 0) {

          GUI.put_string_at(4 + i * 3, number_to_note.notes_upper[note]);
          GUI.put_value_at1(4 + i * 3 + 2, oct);

        } else {
          GUI.put_string_at(4 + i * 3, number_to_note.notes_lower[note]);
          GUI.put_value_at1(4 + i * 3 + 2, oct);
        }
      }
    }
  } else {
    GUI.put_value_at1(15, page_select + 1);
    GUI.put_value_at(6, encoders[2]->getValue());

    if (grid_page.cur_col < 16) {
      GUI.put_p_string_at(10, str1);
      GUI.put_p_string_at(12, str2);
    } else {
      GUI.put_value_at(6,
                       (encoders[2]->getValue() /
                        (2 / mcl_seq.ext_tracks[last_ext_track].resolution)));
      if (Analog4.connected) {
        GUI.put_string_at(10, "A4T");
      } else {
        GUI.put_string_at(10, "MID");
      }
      GUI.put_value_at1(13, last_ext_track + 1);
    }
  }
  draw_pattern_mask((page_select * 16), DEVICE_A4);
}

bool SeqExtStepPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    uint8_t device = midi_active_peering.get_device(port);

    uint8_t track = event->source - 128;

    if (device == DEVICE_A4) {
      uint8_t track = event->source - 128 - 16;
    }

    if (event->mask == EVENT_BUTTON_PRESSED) {

      if (device == DEVICE_MD) {

        if ((track + (page_select * 16)) >=
            mcl_seq.ext_tracks[last_ext_track].length) {
          note_interface.notes[track] = 0;
          return;
        }

        int8_t utiming = mcl_seq.ext_tracks[last_ext_track]
                             .timing[(track + (page_select * 16))]; // upper
        uint8_t condition =
            mcl_seq.ext_tracks[last_ext_track]
                .conditional[(track + (page_select * 16))]; // lower
        encoders[0]->cur = condition;
        // Micro
        if (utiming == 0) {
          if (mcl_seq.ext_tracks[last_ext_track].resolution == 1) {
            utiming = 6;
            ((MCLEncoder *)encoders[1])->max = 11;
          } else {
            ((MCLEncoder *)encoders[1])->max = 23;
            utiming = 12;
          }
        }
        encoders[1]->cur = utiming;

        note_interface.last_note = track;
      }
      if (device == DEVICE_A4) {
        last_ext_track = track;
        init();
      }
    }
    if (event->mask == EVENT_BUTTON_RELEASED) {
      uint8_t utiming = (encoders[1]->cur + 0);
      uint8_t condition = encoders[0]->cur;
      if ((track + (page_select * 16)) >=
          mcl_seq.ext_tracks[last_ext_track].length) {
        return true;
      }

      //  timing = 3;
      // condition = 3;
      if ((slowclock - note_interface.note_hold) < TRIG_HOLD_TIME) {
        for (uint8_t c = 0; c < 4; c++) {
          if (mcl_seq.ext_tracks[last_ext_track]
                  .notes[c][track + page_select * 16] > 0) {
            MidiUart2.sendNoteOff(last_ext_track,
                                  abs(mcl_seq.ext_tracks[last_ext_track]
                                          .notes[c][track + page_select * 16]) -
                                      1,
                                  0);
          }
          mcl_seq.ext_tracks[last_ext_track]
              .notes[c][track + page_select * 16] = 0;
        }
        mcl_seq.ext_tracks[last_ext_track]
            .timing[(track + (page_select * 16))] = 0;
        mcl_seq.ext_tracks[last_ext_track]
            .conditional[(track + (page_select * 16))] = 0;
      }

      else {
        mcl_seq.ext_tracks[last_ext_track]
            .timing[(track + (page_select * 16))] = condition; // upper
        mcl_seq.ext_tracks[last_ext_track]
            .timing[(track + (page_select * 16))] = utiming; // upper
      }

      return true;
    }
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.ENCODER1)) {
    md_exploit.off();
    GUI.setPage(&grid_page);
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON2) && BUTTON_DOWN(Buttons.BUTTON3)) {
    if (mcl_seq.ext_tracks[last_ext_track].resolution == 1) {
      mcl_seq.ext_tracks[last_ext_track].resolution = 2;
      init();

    } else {
      mcl_seq.ext_tracks[last_ext_track].resolution = 1;
      init();
    }

    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    GUI.setPage(&seq_step_page);
    return true;
  }
  if (EVENT_RELEASED(event, Buttons.BUTTON4)) {
    mcl_seq.ext_tracks[last_ext_track].clear_track();
    return true;
  }
  if ((EVENT_PRESSED(event, Buttons.BUTTON3) && BUTTON_DOWN(Buttons.BUTTON4)) ||
      (EVENT_PRESSED(event, Buttons.BUTTON4) && BUTTON_DOWN(Buttons.BUTTON3))) {
    for (uint8_t n = 0; n < mcl_seq.num_ext_tracks; n++) {
      mcl_seq.ext_tracks[last_ext_track].clear_track();
    }
    return true;
  }
  if (SeqPage::handleEvent(event)) {
    return true;
  }

  return false;
}

void SeqExtStepMidiEvents::onNoteOnCallback_Midi2(uint8_t *msg) {
  // Step edit for ExtSeq
  // For each incoming note, check to see if note interface has any steps
  // selected For selected steps record notes.

  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  last_ext_track = channel;
  grid_page.cur_col = 16 + channel;

  for (uint8_t i = 0; i < 16; i++) {
    if (note_interface.notes[i] == 1) {
      mcl_seq.ext_tracks[channel].set_ext_track_step(seq_extstep_page.page_select * 16 + i, msg[1], msg[2]);
    }
  }
}

void SeqExtStepMidiEvents::onNoteOffCallback_Midi2(uint8_t *msg) {}

void SeqExtStepMidiEvents::setup_callbacks() {
  if (state) {
    return;
  }
  Midi.addOnNoteOnCallback(
      this, (midi_callback_ptr_t)&SeqExtStepMidiEvents::onNoteOnCallback_Midi2);
  Midi.addOnNoteOffCallback(
      this,
      (midi_callback_ptr_t)&SeqExtStepMidiEvents::onNoteOffCallback_Midi2);

  state = true;
}

void SeqExtStepMidiEvents::remove_callbacks() {

  if (!state) {
    return;
  }
  Midi.removeOnNoteOnCallback(
      this, (midi_callback_ptr_t)&SeqExtStepMidiEvents::onNoteOnCallback_Midi2);
  Midi.removeOnNoteOffCallback(
      this,
      (midi_callback_ptr_t)&SeqExtStepMidiEvents::onNoteOffCallback_Midi2);
  state = false;
}
