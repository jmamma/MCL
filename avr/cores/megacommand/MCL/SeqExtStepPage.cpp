#include "SeqExtStepPage.h"

void SeqExtStepPage::setup() { SeqPage::setup(); }
void SeqExtStepPage::init() {
  if (ExtPatternResolution[last_ext_track] == 1) {
    encoders[2]->cur = 6;
    encoders[2]->max = 11;
  } else {
    encoders[2]->cur = 12;
    encoders[2]->max = 23;
  }
  encoders[3]->cur = ExtPatternLengths[track];
  cur_col = last_ext_track + 16;
  curpage = SEQ_EXTSTEP_PAGE;
}
void SeqExtStepPage::pattern_len_handler(Encoder *enc) {
  if (BUTTON_DOWN(Buttons.BUTTON3)) {
    for (uint8_t c = 0; c < 6; c++) {
      ExtPatternLengths[c] = encoders[3]->getValue();
    }
  }
  ExtPatternLengths[last_Ext_track] = encoders[3]->getValue();
}

bool SeqExtStepPage::display() {
  GUI.put_string_at(0, "                ");

  const char *str1 = getMachineNameShort(MD.kit.models[last_md_track], 1);
  const char *str2 = getMachineNameShort(MD.kit.models[last_md_track], 2);

  char c[3] = "--";

  if (encoders[1]->getValue() == 0) {
    GUI.put_string_at(0, "L1");

  } else if (encoders[1]->getValue() <= 8) {
    GUI.put_string_at(0, "L");

    GUI.put_value_at1(1, encoders[1]->getValue());

  } else {
    GUI.put_string_at(0, "P");
    uint8_t prob[5] = {1, 2, 5, 7, 9};
    GUI.put_value_at1(1, prob[encoders[1]->getValue() - 9]);
  }

  // Cond
  //    GUI.put_value_at2(0, encoders[1]->getValue());
  // Pos
  // 0  1   2  3  4  5  6  7  8  9  10  11
  //  -5  -4 -3 -2 -1 0
  if (ExtPatternResolution[last_Ext_track] == 1) {
    if (encoders[2]->getValue() == 0) {
      GUI.put_string_at(2, "--");
    } else if ((encoders[2]->getValue() < 6) &&
               (encoders[2]->getValue() != 0)) {
      GUI.put_string_at(2, "-");
      GUI.put_value_at1(3, encoders[2]->getValue() - 6);
    } else {
      GUI.put_string_at(2, "+");
      GUI.put_value_at1(3, encoders[2]->getValue() - 6);
    }
  } else {
    if (encoders[2]->getValue() == 0) {
      GUI.put_string_at(2, "--");
    } else if ((encoders[2]->getValue() < 12) &&
               (encoders[2]->getValue() != 0)) {
      GUI.put_string_at(2, "-");
      GUI.put_value_at1(3, 12 - encoders[2]->getValue());

    } else {
      GUI.put_string_at(2, "+");
      GUI.put_value_at1(3, encoders[2]->getValue() - 12);
    }
  }

  struct musical_notes number_to_note;
  uint8_t notenum;
  uint8_t notes_held = 0;
  uint8_t i;
  for (i = 0; i < 16; i++) {
    if (notes[i] == 1) {
      notes_held += 1;
    }
  }

  if (notes_held > 0) {
    for (i = 0; i < 4; i++) {

      notenum =
          abs(ExtPatternNotes[last_Ext_track][i]
                             [note_interface.last_note + seq_page.page_select * 16]);
      if (notenum != 0) {
        notenum = notenum - 1;
        uint8_t oct = notenum / 12;
        uint8_t note = notenum - 12 * (notenum / 12);
        if (ExtPatternNotes[last_Ext_track][i][note_interface.last_note +
                                               seq_page.page_select * 16] > 0) {

          GUI.put_string_at(4 + i * 3, number_to_note.notes_upper[note]);
          GUI.put_value_at1(4 + i * 3 + 2, oct);

        } else {
          GUI.put_string_at(4 + i * 3, number_to_note.notes_lower[note]);
          GUI.put_value_at1(4 + i * 3 + 2, oct);
        }
      }
    }
  } else {
    GUI.put_value_at1(15, seq_page.page_select + 1);
    GUI.put_value_at(6, encoders[3]->getValue());

    if (cur_col < 16) {
      GUI.put_p_string_at(10, str1);
      GUI.put_p_string_at(12, str2);
    } else {
      GUI.put_value_at(6, (encoders[3]->getValue() /
                           (2 / ExtPatternResolution[last_Ext_track])));
      if (Analog4.connected) {
        GUI.put_string_at(10, "A4T");
      } else {
        GUI.put_string_at(10, "MID");
      }
      GUI.put_value_at1(13, last_Ext_track + 1);
    }
  }
  // PatternLengths[cur_col] = encoders[3]->getValue();
  draw_patternmask((seq_page.page_select * 16), DEVICE_A4);
}

bool SeqExtStepPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    uint8_t device = midi_active_peering.get_device(port);

    if (device == MD_DEVICE) {
      uint8_t track = event->source - 128;
    }
    if (device == A4_DEVICE) {
      uint8_t track = event->source - 128 - 16;
    }

    if (event->mask == EVENT_BUTTON_PRESSED) {

      if (port == UART2_PORT) {
        encoders[3]->cur = ExtPatternLengths[channel];

        last_Ext_track = channel;
        cur_col = 16 + channel;

        for (uint8_t i = 0; i < 16; i++) {
          uint8_t match = 255;
          if (notes[i] == 1) {
            // Look for matching note already on this step
            // If it's a note off, then disable the note
            // If it's a note on, set the note note-off.
            for (uint8_t c = 0; c < 4 && match == 255; c++) {
              if (ExtPatternNotes[channel][c][i + seq_page.page_select * 16] ==
                  -(1 * (msg[1] + 1))) {
                ExtPatternNotes[channel][c][i + seq_page.page_select * 16] = 0;
                mcl_seq.seq_buffer_notesoff(channel);
                match = c;
              }
              if (ExtPatternNotes[channel][c][i + seq_page.page_select * 16] ==
                  (1 * (msg[1] + 1))) {
                ExtPatternNotes[channel][c][i + seq_page.page_select * 16] =
                    (-1 * (msg[1] + 1));
                match = c;
              }
            }

            // No matches are found, we count number of on and off to determine
            // next note type.
            for (uint8_t c = 0; c < 4 && match == 255; c++) {
              if (ExtPatternNotes[channel][c][i + seq_page.page_select * 16] == 0) {
                match = c;
                int8_t ons_and_offs = 0;
                // Check to see if we have same number of note offs as note ons.
                // If there are more note ons for given note, the next note
                // entered should be a note off.
                for (uint8_t a = 0; a < ExtPatternLengths[channel]; a++) {
                  for (uint8_t b = 0; b < 4; b++) {
                    if (ExtPatternNotes[channel][b][a] == -(1 * (msg[1] + 1))) {
                      ons_and_offs -= 1;
                    }
                    if (ExtPatternNotes[channel][b][a] == (1 * (msg[1] + 1))) {
                      ons_and_offs += 1;
                    }
                  }
                }
                if (ons_and_offs <= 0) {
                  ExtPatternNotes[channel][c][i + seq_page.page_select * 16] =
                      (msg[1] + 1);
                } else {
                  ExtPatternNotes[channel][c][i + seq_page.page_select * 16] =
                      -1 * (msg[1] + 1);
                }
              }
            }
          }
        }

        return;
      }
      if (device == MD_DEVICE) {

        if ((note_num + (seq_page.page_select * 16)) >=
            ExtPatternLengths[last_extseq_track]) {
          notes[note_num] = 0;
          return;
        }

        int8_t utiming =
            Exttiming[last_extseq_track]
                     [(note_num + (seq_page.page_select * 16))]; // upper
        uint8_t condition =
            Extconditional[last_extseq_track]
                          [(note_num + (seq_page.page_select * 16))]; // lower
        encoders[1]->cur = condition;
        // Micro
        if (utiming == 0) {
          if (ExtPatternResolution[last_extseq_track] == 1) {
            utiming = 6;
            encoders[2]->max = 11;
          } else {
            encoders[2]->max = 23;
            utiming = 12;
          }
        }
        encoders[2]->cur = utiming;

        note_interface.last_note = note_num;
      }
      if (device == A4_DEVICE) {
        last_Ext_track = track;
        init();
      }
    }
    if (event->mask == EVENT_BUTTON_RELEASED) {
    }
    return true;
  }

 if (EVENT_PRESSED(event, Buttons.ENCODER1) {
    md_exploit.off();
    GUI.setPage(&grid_page);
    curpage = GRID_PAGE;
    return true;

  }

if ((EVENT_PRESSED(event, Buttons.BUTTON1) && BUTTON_DOWN(Buttons.BUTTON4)) ||
    (EVENT_PRESSED(event, Buttons.BUTTON4) && BUTTON_DOWN(Buttons.BUTTON3))) {
    for (uint8_t n = 0; n < 16; n++) {
      clear_seq_track(n);
    }
    return true;
}

if ( EVENT_PRESSED(event, Buttons.BUTTON2) && BUTTON_DOWN(Buttons.BUTTON3)) {
    if (ExtPatternResolution[last_ext_track] == 1) {
      ExtPatternResolution[last_ext_track] = 2;
      if (curpage == SEQ_EXTSTEP_PAGE) {
        GUI.setPage(seq_extstep_page);
      }

    } else {
      ExtPatternResolution[last_ext_track] = 1;
      if (curpage == SEQ_EXTSTEP_PAGE) {
        GUI.setPage(seq_extstep_page);
      }
    }

    return true;
}

  if (EVENT_RELEASED(event, Buttons.BUTTON1))  {
    GUI.setPage(seq_step_page);
    return true;
  }
if (EVENT_RELEASED(event, Buttons.BUTTON4)) {
    clear_Ext_track(last_Ext_track);
    return true;
  }

  if (SeqExtStep::handleEvent(event)) {
    return true;
  }


return false;
}
