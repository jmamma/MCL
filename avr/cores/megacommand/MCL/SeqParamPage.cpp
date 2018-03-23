#include "SeqParamPage.h"

void SeqParamPage::setup() { SeqPage::setup(); }
void SeqParamPage::init() {
  md_exploit.on();
  note_interface.state = true;

  encoders[0]->max = 23;
  encoders[1]->max = 127;
  encoders[2]->max = 23;
  encoders[3]->max = 127;

  encoders[0]->cur = mcl_seq.md_tracks[last_md_track].locks_params[p1];
  encoders[2]->cur = mcl_seq.md_tracks[last_md_track].locks_params[p2];
  encoders[1]->cur =
      MD.kit.params[last_md_track][mcl_seq.md_tracks[last_md_track].locks_params[p1]];
  encoders[3]->cur =
      MD.kit.params[last_md_track][mcl_seq.md_tracks[last_md_track].locks_params[p2]];

  curpage = SEQ_PARAMA_PAGE;
}
void SeqParamPage::construct(uint8_t p1, uint8_t p2) {
  param1 = p1;
  param2 = p2;
}
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
  GUI.put_value_at2(4, encoders[1]->getValue());
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

  GUI.put_value_at2(11, encoders[3]->getValue());

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
    if (device == DEVICE_A4) {
       return true;
    }

    if (event->mask == EVENT_BUTTON_PRESSED) {
      uint8_t param_offset;
      encoders[0]->cur = mcl_seq.md_tracks[last_md_track].lock_params[p1];
      encoders[2]->cur = mcl_seq.md_tracks[last_md_track].lock_params[p2];

      encoders[1]->cur = mcl_seq.md_tracks[last_md_track].locks[p1][(track + (seq_page.page_select * 16))];
      encoders[3]->cur = mcl_seq.md_tracks[last_md_track].locks[p2]
                                     [(track + (seq_page.page_select * 16))];
      notes[track] = 1;
    }
    if (event->mask == EVENT_BUTTON_RELEASED) {
      int8_t utiming =
          mcl_seq.md_tracks[grid.cur_col].timing[(track + (seq_page.page_select * 16))]; // upper
      uint8_t condition =
          mcl_seq.md_tracks[grid.cur_col].conditional
                     [(track + (seq_page.page_select * 16))]; // lower

      // Fudge timing info if it's not there
      if (utiming == 0) {
        utiming = 12;
         mcl_seq.md_tracks[last_md_track].conditional[(track + (seq_page.page_select * 16))] =
            condition;
       mcl_seq.md_tracks[last_md_track].timing[(track + (seq_page.page_select * 16))] =
            utiming;
      }
      if (IS_BIT_SET64(mcl_seq.md_tracks[last_md_track].lockmask,
                       (track + (seq_page.page_select * 16)))) {
        if ((current_clock - note_hold) < 300) {
          CLEAR_BIT64(mcl_seq.md_tracks[last_md_track].lockmask,
                      (track + (seq_page.page_select * 16)));
        }
      } else {
        SET_BIT64(mcl_seq.md_tracks[last_md_track].lockmask,
                  (track + (seq_page.page_select * 16)));
      }

      notes[track] = 0;

      mcl_seq.md_tracks[last_md_track].locks[param_offset][(track + (seq_page.page_select * 16))] = encoders[1]->cur;
      mcl_seq.md_tracks[last_md_track].locks[param_offset + 1]
                  [(track + (seq_page.page_select * 16))] = encoders[3]->cur;

      mcl_seq.md_tracks[last_md_track].params[p1] = encoders[0]->cur;
      mcl_seq.md_tracks[last_md_track].params[p2] = encoders[2]->cur;
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
    mcl_seq.md_tracks[last_md_track].clear_seq_track();
    return true;
  }

  if ((EVENT_PRESSED(evnt, Buttons.BUTTON1) && BUTTON_DOWN(Buttons.BUTTON4)) ||
      (EVENT_PRESSED(evnt, Buttons.BUTTON4) && BUTTON_DOWN(Buttons.BUTTON3))) {

    for (uint8_t n = 0; n < 16; n++) {
      mcl_seq.md_tracks[n].clear_seq_locks();
    }
    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    encoders[0]->cur = mcl_seq.md_tracks[last_md_track].params[p1];
    encoders[2]->cur = mcl_seq.md_tracks[last_md_track].params[p2];
    encoders[1]->cur =
        MD.kit.params[last_md_track][mcl_seq.md_tracks[last_md_track].params[p1]];
    encoders[3]->cur =
        MD.kit.params[last_md_track][mcl_seq.md_tracks[last_md_track].params[p1]];
    curpage = SEQ_PARAM_B_PAGE;
    return true;
  }
  if (EVENT_RELEASED(event, Buttons.BUTTON4)) {
    mcl_seq.md_tracks[grid.cur_col].clear_seq_locks();
    return true;
  }

  if (SeqPage::handleEvent(event)) {
    return true;
  }

  return false;
}
