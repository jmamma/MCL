#include "SeqPage.h"

bool SeqPage::handleEvent(gui_event_t *event) {
  //  if (note_interface.is_event(event)) {

  //  return true;
  //  }
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

  if (EVENT_PRESSED(evt, Buttons.BUTTON2) ) {
    uint8_t pagemax = 4;
    seq_page_select += 1;

    if (grid.cur_col > 15) {
      pagemax = 8;

    }
    if (seq_page_select >= pagemax) {
      seq_page_select = 0;
    }

    return true;

  }

  return false;
}
SeqPage::draw_lockmask(uint8_t offset) {
  GUI.setLine(GUI.LINE2);

  char str[17] = "----------------";
  uint8_t step_count =
      (MidiClock.div16th_counter - pattern_start_clock32th / 2) -
      (PatternLengths[grid.cur_col] *
       ((MidiClock.div16th_counter - pattern_start_clock32th / 2) /
        PatternLengths[grid.cur_col]));

  for (int i = 0; i < 16; i++) {

    if (i + offset >= PatternLengths[grid.cur_col]) {
      str[i] = ' ';
    } else if ((step_count == i + offset) && (MidiClock.state == 2)) {
      str[i] = ' ';
    } else {
      if (IS_BIT_SET64(LockMasks[grid.cur_col], i + offset)) {
        str[i] = 'x';
      }
      if (IS_BIT_SET64(PatternMasks[grid.cur_col], i + offset) &&
          !IS_BIT_SET64(LockMasks[grid.cur_col], i + offset)) {

        str[i] = (char)165;
      }
      if (IS_BIT_SET64(PatternMasks[grid.cur_col], i + offset) &&
          IS_BIT_SET64(LockMasks[grid.cur_col], i + offset)) {

        str[i] = (char)219;
      }

      if (notes[i] > 0) {
        /*If the bit is set, there is a cue at this position. We'd like to
         * display it as [] on screen*/
        /*Char 219 on the minicommand LCD is a []*/

        str[i] = (char)255;
      }
    }
  }
  GUI.put_string_at(0, str);
}

SeqPage::draw_patternmask(uint8_t offset, uint8_t device) {
  GUI.setLine(GUI.LINE2);
  /*str is a string used to display 16 steps of the patternmask at a time*/
  /*blank steps sequencer trigs are denoted by a - */

  /*Initialise the string with blank steps*/
  char mystr[17] = "----------------";

  /*Get the Pattern bit mask for the selected track*/
  //    uint64_t patternmask = getPatternMask(grid.cur_col, grid.cur_row , 3, false);
  uint64_t patternmask = PatternMasks[grid.cur_col];
  int8_t note_held = 0;

  /*Display 16 steps on screen, starting at an offset set by the encoder1
   * value*/
  /*The encoder offset allows you to scroll through the 4 pages of the 16 step
   * sequencer triggers that make up a 64 step pattern*/

  /*For 16 steps check to see if there is a trigger at pattern position i +
   * (encoder_offset * 16) */
  if (device == DEVICE_MD) {

    for (int i = 0; i < 16; i++) {
      if (device == DEVICE_MD) {
        uint8_t step_count =
            (MidiClock.div16th_counter - pattern_start_clock32th / 2) -
            (PatternLengths[grid.cur_col] *
             ((MidiClock.div16th_counter - pattern_start_clock32th / 2) /
              PatternLengths[grid.cur_col]));

        if (i + offset >= PatternLengths[grid.cur_col]) {
          mystr[i] = ' ';
        } else if ((step_count == i + offset) && (MidiClock.state == 2)) {
          mystr[i] = ' ';
        } else if (notes[i - offset] > 0) {
          /*If the bit is set, there is a cue at this position. We'd like to
           * display it as [] on screen*/
          /*Char 219 on the minicommand LCD is a []*/

          mystr[i - offset] = (char)255;
        } else if (IS_BIT_SET64(patternmask, i + offset)) {
          /*If the bit is set, there is a trigger at this position. We'd like to
           * display it as [] on screen*/
          /*Char 219 on the minicommand LCD is a []*/
          mystr[i] = (char)219;
        }
      }
    }
  } else {

    for (int i = 0; i < ExtPatternLengths[last_Ext_track]; i++) {

      uint8_t step_count =
          ((MidiClock.div32th_counter / ExtPatternResolution[last_Ext_track]) -
           (pattern_start_clock32th / ExtPatternResolution[last_Ext_track])) -
          (ExtPatternLengths[last_Ext_track] *
           ((MidiClock.div32th_counter / ExtPatternResolution[last_Ext_track] -
             (pattern_start_clock32th / ExtPatternResolution[last_Ext_track])) /
            (ExtPatternLengths[last_Ext_track])));
      uint8_t noteson = 0;
      uint8_t notesoff = 0;

      for (uint8_t a = 0; a < 4; a++) {

        if (ExtPatternNotes[last_Ext_track][a][i] > 0) {

          noteson++;
          //    mystr[i] = (char) 219;
        }

        if (ExtPatternNotes[last_Ext_track][a][i] < 0) {
          notesoff++;
        }
      }
      if ((noteson > 0) && (notesoff > 0)) {
        if ((i >= offset) && (i < offset + 16)) {
          mystr[i - offset] = (char)005;
        }
        note_held += noteson;
        note_held -= notesoff;

      } else if (noteson > 0) {
        if ((i >= offset) && (i < offset + 16)) {
          mystr[i - offset] = (char)002;
        }
        note_held += noteson;
      } else if (notesoff > 0) {
        if ((i >= offset) && (i < offset + 16)) {
          mystr[i - offset] = (char)004;
        }
        note_held -= notesoff;
      } else {
        if (note_held >= 1) {
          if ((i >= offset) && (i < offset + 16)) {
            mystr[i - offset] = (char)003;
          }
        }
      }

      if ((i >= ExtPatternLengths[last_Ext_track]) ||
          (step_count == i) && (MidiClock.state == 2)) {
        if ((i >= offset) && (i < offset + 16)) {
          mystr[i - offset] = ' ';
        }
      }
      if ((i >= offset) && (i < offset + 16)) {

        if (notes[i - offset] > 0) {
          /*If the bit is set, there is a cue at this position. We'd like to
           * display it as [] on screen*/
          /*Char 219 on the minicommand LCD is a []*/

          mystr[i - offset] = (char)255;
        }
      }
    }
  }

  /*Display the step sequencer pattern on screen, 16 steps at a time*/
  GUI.put_string_at(0, mystr);
}


void SeqPage::pattern_len_handler(Encoder *enc) {
    if (grid.cur_col < 16) {
      PatternLengths[grid.cur_col] = encoders[3]->getValue();
      if (BUTTON_DOWN(Buttons.BUTTON3)) {
        for (uint8_t c = 0; c < 16; c++) {
          PatternLengths[c] = encoders[3]->getValue();
        }
      }

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
void SeqPage::create_char_seq() {
  uint8_t temp_charmap1[8] = {  0, 15, 16, 16, 16, 15, 0 };
  uint8_t temp_charmap2[8] = {  0, 31, 0, 0, 0, 31, 0 };
  uint8_t temp_charmap3[8] = {  0, 30, 1, 1, 1, 30, 0 };
  uint8_t temp_charmap4[8] = {  0, 27, 4, 4, 4, 27, 0 };
  LCD.createChar(2, temp_charmap1);
  LCD.createChar(3, temp_charmap2);
  LCD.createChar(4, temp_charmap3);
  LCD.createChar(5, temp_charmap4);

}

void SeqPage::display() {
    GUI.setLine(GUI.LINE1);
    GUI.put_value_at1(15, seq_page_select + 1);
}

void SeqPage::setup() {

  encoders[3]->handler = pattern_len_handler;
  encoders[2]->min = 0;
  if (curpage == 0) {
    create_chars_seq();
    currentkit_temp = MD.getCurrentKit(CALLBACK_TIMEOUT);
    // curpage = page;

    // Don't save kit if sequencer is running, otherwise parameter locks will be
    // stored.
    if (MidiClock.state != 2) {
      MD.saveCurrentKit(currentkit_temp);
    }

    MD.getBlockingKit(currentkit_temp);
    MD.getCurrentTrack(CALLBACK_TIMEOUT);

    md_exploit.on();
  } else {
    md_exploit.init_notes();
  }
  grid.cur_col = last_md_track;
  grid.cur_row = param2.getValue();
}

void SeqPageMidiEvents::setup_callbacks() {
  Midi.addOnControlChangeCallback(
      this, (midi_callback_ptr_t)&SeqPageMidiEvents::onControlChangeCallback);
}

void SeqPageMidiEvents::remove_callbacks() {
  Midi.removeOnControlChangeCallback(
      this, (midi_callback_ptr_t)&SeqPageMidiEvents::onControlChangeCallback);
}

void SeqPageMidiEvents::onControlChangeCallback_Midi(uint8_t *msg) {
  if (md_exploit.off()) {
    GUI.setPage(&page);
    curpage = 0;
  }
}
