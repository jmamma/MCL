#include "SeqPtcPage.h"

void SeqPtcPage::setup() {
  SeqPage::setup();
}
void SeqPtcPage::init() {
  collect_trigs = false;

  encoders[1]->max = 8;
  encoders[2]->max = 64;
  encoders[3]->max = 64;
  encoders[4]->max = 15;
  encoders[2]->cur = 32;
  encoders[1]->cur = 1;

  encoders[3]->cur = PatternLengths[last_md_track];

  curpage = SEQ_PTC_PAGE;

}
void SeqPtcPage::pattern_len_handler(Encoder *enc) {
  if (grid.cur_col < 16) {
      if (BUTTON_DOWN(Buttons.BUTTON3)) {
        for (uint8_t c = 0; c < 16; c++) {
          PatternLengths[c] = encoders[3]->getValue();
        }
      }
      PatternLengths[last_md_track] = encoders[3]->getValue();
    }
    else {
      if (BUTTON_DOWN(Buttons.BUTTON3)) {
        for (uint8_t c = 0; c < 6; c++) {
          ExtPatternLengths[c] = encoders[3]->getValue();
        }
      }
      ExtPatternLengths[last_Ext_track] = encoders[3]->getValue();
    }
  }
  md_exploit.init_notes();
}

bool SeqPtcPage::display() {
  const char *str1 = getMachineNameShort(MD.kit.models[grid.cur_col], 1);
  const char *str2 = getMachineNameShort(MD.kit.models[grid.cur_col], 2);
  GUI.setLine(GUI.LINE1);
  if ((curpage == SEQ_RPTC_PAGE)) {
  } else {
    GUI.put_string_at(0, "PTC");
  }
  if (grid.cur_col < 16) {
    GUI.put_value_at(5, encoders[3]->getValue());
    GUI.put_p_string_at(9, str1);
    GUI.put_p_string_at(11, str2);
  } else {
    GUI.put_value_at(5, (encoders[3]->getValue() /
                         (2 / ExtPatternResolution[last_Ext_track])));
    if (Analog4.connected) {
      GUI.put_string_at(9, "A4T");
    } else {
      GUI.put_string_at(9, "MID");
    }
    GUI.put_value_at1(12, grid.cur_col - 16 + 1);
  }
  GUI.setLine(GUI.LINE2);
  GUI.put_string_at(0, "OC:");
  GUI.put_value_at2(3, encoders[1]->getValue());

  if (encoders[2]->getValue() < 32) {
    GUI.put_string_at(6, "F:-");
    GUI.put_value_at2(9, 32 - encoders[2]->getValue());

  } else if (encoders[2]->getValue() > 32) {
    GUI.put_string_at(6, "F:+");
    GUI.put_value_at2(9, encoders[2]->getValue() - 32);

  } else {
    GUI.put_string_at(6, "F: 0");
  }

  GUI.put_string_at(12, "S:");

  GUI.put_value_at2(14, encoders[4]->getValue());
}
bool SeqPtcPage::handleEvent(gui_event_t *event) {

  if (note_interface.is_event(event)) {
    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    curpage = SEQ_RPTC_PAGE;
    GUI.setPage(&seq_rptc_page);
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.ENCODER4)) {
    md_exploit.off();
    GUI.setPage(&grid_page);
    curpage = GRID_PAGE;
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.ENCODER1)) {

    load_seq_page(SEQ_STEP_PAGE);
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER2)) {

    load_seq_page(SEQ_RTRK_PAGE);

    return true;
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER3)) {
    load_seq_page(SEQ_PARAM_A_PAGE);

    return true;
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER4)) {

    load_seq_page(SEQ_PTC_PAGE);

    return true;
  }
  if ((EVENT_PRESSED(event, Buttons.BUTTON1) && BUTTON_DOWN(Buttons.BUTTON4)) ||
      (EVENT_PRESSED(event, Buttons.BUTTON4) && BUTTON_DOWN(Buttons.BUTTON3))) {

    for (uint8_t n = 0; n < 6; n++) {
      clear_Ext_track(n);
    }

    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON2) && BUTTON_DOWN(Buttons.BUTTON3)) {
    if (ExtPatternResolution[last_ext_track] == 1) {
      ExtPatternResolution[last_ext_track] = 2;
      if (curpage == SEQ_EXTSTEP_PAGE) {
        load_seq_extstep_page(last_ext_track);
      }

    } else {
      ExtPatternResolution[last_ext_track] = 1;
      if (curpage == SEQ_EXTSTEP_PAGE) {
        load_seq_extstep_page(last_ext_track);
      }
    }

    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON4)) {

    if (grid.cur_col < 16) {
      clear_seq_track(last_md_track);
    } else {
      clear_Ext_track(last_Ext_track);
    }
    return true;
  }

  if (SeqPage::handleEvent(event)) {
    return true;
  }

  return false;
}
