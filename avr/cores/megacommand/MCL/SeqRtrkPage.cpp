#include "SeqRtrkPage.h"

void SeqRtrkPage::setup() { SeqPage::setup(); }

void SeqRtrkPage::init() {
  collect_trigs = false;

  encoders[1]->max = 4;
  encoders[2]->max = 64;
  encoders[3]->max = 64;
  encoders[4]->max = 11;
  encoders[3]->cur = PatternLengths[last_md_track];

  curpage = SEQ_RTRK_PAGE;
}

bool SeqRtrkPage::display() {

  GUI.setLine(GUI.LINE1);
  GUI.put_value_at1(15, seq_page.page_select + 1);

  GUI.put_string_at(0, "RTRK");

  const char *str1 = getMachineNameShort(MD.kit.models[grid.cur_col], 1);
  const char *str2 = getMachineNameShort(MD.kit.models[grid.cur_col], 2);
  if (grid.cur_col < 16) {
    GUI.put_p_string_at(9, str1);
    GUI.put_p_string_at(11, str2);
    GUI.put_value_at(5, encoders[3]->getValue());
  } else {
    GUI.put_value_at(5, (encoders[3]->getValue() /
                         (2 / ExtPatternResolution[last_ext_track])));
    if (Analog4.connected) {
      GUI.put_string_at(9, "A4T");
    } else {
      GUI.put_string_at(9, "MID");
    }
    GUI.put_value_at1(12, grid.cur_col - 16 + 1);
  }

  draw_patternmask(seq_page.page_select * 16, DEVICE_MD);
}
bool SeqRtrkPage::handleEvent(gui_event_t *event) {

  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t device = midi_active_peering.get_device(port);

    uint8_t track = event->source - 128;

    if (event->mask == EVENT_BUTTON_PRESSED) {

      MD.triggerTrack(note_num, 127);
    if ((record && (MidiClock.state == 2)) {
        mcl_seq.rec_track(track, note_num, 127);

        return true;

    }
    }
    if (event->mask == EVENT_BUTTON_RELEASED) {
    }
  }
  if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    md_exploit.off();
    curpage = SEQ_RLCK_PAGE;
    GUI.setPage(&seq_rlck_page);
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.ENCODER2)) {
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

  if (EVENT_RELEASED(Event, Buttons.BUTTON4)) {
    if (grid.cur_col < 16) {
      clear_seq_track(grid.cur_col);
    } else {
      mcl_seq.clear_ext_track(grid.cur_col - 16);
    }
    return true;
  }

  if (SeqPage::handleEvent(event)) {
    return true;
  }

  return false;
}
SeqRtrkPage seq_rtrk_page;
