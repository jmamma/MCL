#include "SeqStepPage.h"

void SeqStepPage::setup() { SeqPage::setup(); }
void SeqStepPage::init() {
  md_exploit.on();
  note_interface.state = true;

  encoders[0]->max = 13;
  encoders[1]->max = 23;
  encoders[1]->min = 1;
  encoders[1]->cur = 12;
  encoders[2]->max = 64;
  encoders[3]->max = 16;
  encoders[2]->cur = mcl_seq.md_tracks[last_md_track];

  curpage = SEQ_STEP_PAGE;
}
void SeqStepPage::display() {
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
    GUI.put_value_at1(3, 12 - encoders[1]->getValue());

  } else {
    GUI.put_string_at(2, "+");
    GUI.put_value_at1(3, encoders[1]->getValue() - 12);
  }

  GUI.put_p_string_at(10, str1);
  GUI.put_p_string_at(12, str2);
  GUI.put_value_at(6, encoders[2]->getValue());
  GUI.put_value_at1(15, seq_page.page_select + 1);
  // GUI.put_value_at2(7, encoders[3]->getValue());
  draw_patternmask((seq_page.page_select * 16), DEVICE_MD);
}

bool SeqStepPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t device = midi_active_peering.get_device(port);

    uint8_t track = event->source - 128;

    if (event->mask == EVENT_BUTTON_PRESSED) {

      if (device == DEVICE_A4) {
        last_Ext_Track = track;
        GUI.setPage(&seq_extstep_page);
      }
      if ((track + (seq_page.page_select * 16)) >=
          PatternLengths[grid.cur_col]) {
        return;
      }

      encoders[1]->max = 23;
      int8_t utiming =
          timing[grid.cur_col][(track + (seq_page.page_select * 16))]; // upper
      uint8_t condition =
          conditional[grid.cur_col]
                     [(track + (seq_page.page_select * 16))]; // lower

      // Cond
      encoders[0]->cur = condition;
      // Micro
      if (utiming == 0) {
        utiming = 12;
      }
      encoders[1]->cur = utiming;
    }
    if (event->mask == EVENT_BUTTON_RELEASED) {
      if ((track + (seq_page.page_select * 16)) >= mcl_seq.md_tracks[grid.cur_col]) {
        return;
      }
      uint8_t utiming = (encoders[1]->cur + 0);
      uint8_t condition = encoders[0]->cur;

      //  timing = 3;
      // condition = 3;
      conditional[cur_col][(track + (seq_page.page_select * 16))] =
          condition;                                                    // upper
      timing[cur_col][(track + (seq_page.page_select * 16))] = utiming; // upper

      //   conditional_timing[cur_col][(track + (encoders[1]->cur * 16))] =
      //   condition; //lower

      if (!IS_BIT_SET64(PatternMasks[cur_col],
                        (track + (seq_page.page_select * 16)))) {
        SET_BIT64(PatternMasks[cur_col], (track + (seq_page.page_select * 16)));
      } else {
        if ((current_clock - note_hold) < TRIG_HOLD_TIME) {
          CLEAR_BIT64(PatternMasks[cur_col],
                      (track + (seq_page.page_select * 16)));
        }
      }
      // Cond
      // encoders[3]->cur = condition;
      // Microƒ
      // encoders[4]->cur = timing;
      // draw_notes(1);
      return;
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
  if (EVENT_RELEASED(event, Buttons.BUTTON4)) {
    clear_seq_track(last_md_track);
    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON1))  {
    load_seq_extstep_page(last_ext_track);

    return true;
    /*

      return true;
    */


  }
  if (SeqStep::handleEvent(event)) {
    return true;
  }


return false;
}
