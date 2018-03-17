#include "SeqParamPage.h"

void SeqParamPage::setup() { SeqPage::setup(); }
void SeqParamPage::init() {
  collect_trigs = true;

  encoders[1]->max = 23;
  encoders[2]->max = 127;
  encoders[3]->max = 23;
  encoders[4]->max = 127;

  encoders[1]->cur = PatternLocksParams[last_md_track][0];
  encoders[3]->cur = PatternLocksParams[last_md_track][1];
  encoders[2]->cur =
      MD.kit.params[last_md_track][PatternLocksParams[last_md_track][0]];
  encoders[4]->cur =
      MD.kit.params[last_md_track][PatternLocksParams[last_md_track][1]];

  curpage = SEQ_PARAMA_PAGE;
}
void SeqParamPage::construct(uint8_t p1, uint8_t p2) {
  param1 = p1;
  param2 = p2;
}
bool SeqParamPage::display() {
  GUI.setLine(GUI.LINE1);
  char myName[4] = "-- ";
  char myName2[4] = "-- ";
  if (encoders[1]->getValue() == 0) {
    GUI.put_string_at(0, "--");
  } else {
    PGM_P modelname = NULL;
    modelname = model_param_name(MD.kit.models[last_md_track],
                                 encoders[1]->getValue() - 1);
    if (modelname != NULL) {
      m_strncpy_p(myName, modelname, 4);
    }
    GUI.put_string_at(0, myName);
  }
  GUI.put_value_at2(4, encoders[2]->getValue());
  if (encoders[3]->getValue() == 0) {
    GUI.put_string_at(7, "--");
  } else {
    PGM_P modelname = NULL;
    modelname = model_param_name(MD.kit.models[last_md_track],
                                 encoders[3]->getValue() - 1);
    if (modelname != NULL) {
      m_strncpy_p(myName2, modelname, 4);
    }
    GUI.put_string_at(7, myName2);
  }

  GUI.put_value_at2(11, encoders[4]->getValue());

  if (curpage == SEQ_PARAM_A_PAGE) {
    GUI.put_string_at(14, "A");
  }
  if (curpage == SEQ_PARAM_B_PAGE) {
    GUI.put_string_at(14, "B");
  }
  GUI.put_value_at1(15, (seq_page.page_select + 1));
  draw_lockmask(seq_page.page_select * 16);
}

bool SeqParamPage::handleEvent(gui_event_t *event) {

  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t device = midi_active_peering.get_device(port);

    uint8_t track = event->source - 128;
    if (device == A4_DEVICE) {
       return true;
    }

    if (event->mask == EVENT_BUTTON_PRESSED) {
      uint8_t param_offset;
      encoders[1]->cur = PatternLocksParams[last_md_track][p1];
      encoders[3]->cur = PatternLocksParams[last_md_track][p2];

      encoders[2]->cur = PatternLocks[last_md_track][p1]
                                     [(track + (seq_page.page_select * 16))];
      encoders[4]->cur = PatternLocks[last_md_track][p2]
                                     [(track + (seq_page.page_select * 16))];
      notes[track] = 1;
    }
    if (event->mask == EVENT_BUTTON_RELEASED) {
      int8_t utiming =
          timing[cur_col][(track + (seq_page.page_select * 16))]; // upper
      uint8_t condition =
          conditional[cur_col]
                     [(track + (seq_page.page_select * 16))]; // lower

      // Fudge timing info if it's not there
      if (utiming == 0) {
        utiming = 12;
        conditional[last_md_track][(track + (seq_page.page_select * 16))] =
            condition;
        timing[last_md_track][(track + (seq_page.page_select * 16))] =
            utiming;
      }
      if (IS_BIT_SET64(LockMasks[last_md_track],
                       (track + (seq_page.page_select * 16)))) {
        if ((current_clock - note_hold) < 300) {
          CLEAR_BIT64(LockMasks[last_md_track],
                      (track + (seq_page.page_select * 16)));
        }
      } else {
        SET_BIT64(LockMasks[last_md_track],
                  (track + (seq_page.page_select * 16)));
      }

      notes[track] = 0;

      PatternLocks[last_md_track][param_offset]
                  [(track + (seq_page.page_select * 16))] = encoders[2]->cur;
      PatternLocks[last_md_track][param_offset + 1]
                  [(track + (seq_page.page_select * 16))] = encoders[4]->cur;

      PatternLocksParams[last_md_track][p1] = encoders[1]->cur;
      PatternLocksParams[last_md_track][p2] = encoders[3]->cur;
    }
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER3)) {
    md_exploit.off();
    GUI.setPage(&grid_page);
    curpage = GRID_PAGE;
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
    clear_seq_track(grid.cur_col);
    return true;
  }

  if ((EVENT_PRESSED(evnt, Buttons.BUTTON1) && BUTTON_DOWN(Buttons.BUTTON4)) ||
      (EVENT_PRESSED(evnt, Buttons.BUTTON4) && BUTTON_DOWN(Buttons.BUTTON3))) {

    for (uint8_t n = 0; n < 16; n++) {
      clear_seq_locks(n);
    }
    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    encoders[1]->cur = PatternLocksParams[last_md_track][p1];
    encoders[3]->cur = PatternLocksParams[last_md_track][p2];
    encoders[2]->cur =
        MD.kit.params[last_md_track][PatternLocksParams[last_md_track][p1]];
    encoders[4]->cur =
        MD.kit.params[last_md_track][PatternLocksParams[last_md_track][p2]];
    curpage = SEQ_PARAM_B_PAGE;
    return true;
  }
  if (EVENT_RELEASED(event, Buttons.BUTTON4)) {
    clear_seq_locks(grid.cur_col);
    return true;
  }

  if (SeqPage::handleEvent(event)) {
    return true;
  }

  return false;
}
