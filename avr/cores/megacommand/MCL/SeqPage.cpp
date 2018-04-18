#include "MCL.h"
#include "SeqPage.h"

uint8_t SeqPage::page_select = 0;

uint8_t SeqPage::midi_device = DEVICE_MD;

void SeqPage::create_chars_seq() {
  uint8_t temp_charmap1[8] = {0, 15, 16, 16, 16, 15, 0};
  uint8_t temp_charmap2[8] = {0, 31, 0, 0, 0, 31, 0};
  uint8_t temp_charmap3[8] = {0, 30, 1, 1, 1, 30, 0};
  uint8_t temp_charmap4[8] = {0, 27, 4, 4, 4, 27, 0};
  LCD.createChar(6, temp_charmap1);
  LCD.createChar(3, temp_charmap2);
  LCD.createChar(4, temp_charmap3);
  LCD.createChar(5, temp_charmap4);
}

void SeqPage::setup() {
  create_chars_seq();
  if (MD.connected) {
    MD.currentKit = MD.getCurrentKit(CALLBACK_TIMEOUT);
    // stored.
    if (MidiClock.state != 2) {
      MD.saveCurrentKit(MD.currentKit);
    }

    MD.getBlockingKit(MD.currentKit);
    MD.getCurrentTrack(CALLBACK_TIMEOUT);

    ((MCLEncoder *)encoders[1])->min = 0;
    ((MCLEncoder *)encoders[2])->handler = pattern_len_handler;
    grid_page.cur_col = last_md_track;
  }
  grid_page.cur_row = param2.getValue();
}

void SeqPage::init() { seqpage_midi_events.setup_callbacks(); }

bool SeqPage::handleEvent(gui_event_t *event) {
  //  if (note_interface.is_event(event)) {

  //  return true;
  //  }
  if (EVENT_PRESSED(event, Buttons.ENCODER1)) {
    GUI.setPage(&seq_step_page);
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER2)) {
    GUI.setPage(&seq_rtrk_page);

    return true;
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER3)) {

    GUI.setPage(&seq_param_page[0]);
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER4)) {

    GUI.setPage(&seq_ptc_page);

    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    uint8_t pagemax = 4;
    page_select += 1;

    if (grid_page.cur_col > 15) {
      pagemax = 8;
    }
    if (page_select >= pagemax) {
      page_select = 0;
    }

    return true;
  }

  return false;
}
void SeqPage::draw_lock_mask(uint8_t offset) {
  GUI.setLine(GUI.LINE2);

  char str[17] = "----------------";
  uint8_t step_count =
      (MidiClock.div16th_counter - mcl_actions_callbacks.start_clock32th / 2) -
      (mcl_seq.md_tracks[grid_page.cur_col].length *
       ((MidiClock.div16th_counter -
         mcl_actions_callbacks.start_clock32th / 2) /
        mcl_seq.md_tracks[grid_page.cur_col].length));

  for (int i = 0; i < 16; i++) {

    if (i + offset >= mcl_seq.md_tracks[grid_page.cur_col].length) {
      str[i] = ' ';
    } else if ((step_count == i + offset) && (MidiClock.state == 2)) {
      str[i] = ' ';
    } else {
      if (IS_BIT_SET64(mcl_seq.md_tracks[grid_page.cur_col].lock_mask,
                       i + offset)) {
        str[i] = 'x';
      }
      if (IS_BIT_SET64(mcl_seq.md_tracks[grid_page.cur_col].pattern_mask,
                       i + offset) &&
          !IS_BIT_SET64(mcl_seq.md_tracks[grid_page.cur_col].lock_mask,
                        i + offset)) {
#ifdef OLED_DISPLAY
        str[i] = (char)165;
#else
        str[i] = (char)0xF8;
#endif
      }
      if (IS_BIT_SET64(mcl_seq.md_tracks[grid_page.cur_col].pattern_mask,
                       i + offset) &&
          IS_BIT_SET64(mcl_seq.md_tracks[grid_page.cur_col].lock_mask,
                       i + offset)) {
#ifdef OLED_DISPLAY
        str[i] = (char)2;
#else
        str[i] = (char)219;
#endif
      }

      if (note_interface.notes[i] == 1) {
/*If the bit is set, there is a cue at this position. We'd like to
 * display it as [] on screen*/
/*Char 219 on the minicommand LCD is a []*/
#ifdef OLED_DISPLAY
        str[i] = (char)3;
#else
        str[i] = (char)255;
#endif
      }
    }
  }
  GUI.put_string_at(0, str);
}

void SeqPage::draw_pattern_mask(uint8_t offset, uint8_t device) {
  GUI.setLine(GUI.LINE2);
  /*str is a string used to display 16 steps of the pattern_mask at a time*/
  /*blank steps sequencer trigs are denoted by a - */

  /*Initialise the string with blank steps*/
  char mystr[17] = "----------------";

  /*Get the Pattern bit mask for the selected track*/
  //    uint64_t pattern_mask = getPatternMask(grid_page.cur_col,
  //    grid_page.cur_row , 3, false);
  uint64_t pattern_mask = mcl_seq.md_tracks[grid_page.cur_col].pattern_mask;
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
        uint8_t step_count = (MidiClock.div16th_counter -
                              mcl_actions_callbacks.start_clock32th / 2) -
                             (mcl_seq.md_tracks[grid_page.cur_col].length *
                              ((MidiClock.div16th_counter -
                                mcl_actions_callbacks.start_clock32th / 2) /
                               mcl_seq.md_tracks[grid_page.cur_col].length));

        if (i + offset >= mcl_seq.md_tracks[grid_page.cur_col].length) {
          mystr[i] = ' ';
        } else if ((step_count == i + offset) && (MidiClock.state == 2)) {
          mystr[i] = ' ';
        } else if (note_interface.notes[i - offset] == 1) {
          /*Char 219 on the minicommand LCD is a []*/
#ifdef OLED_DISPLAY
          mystr[i] = (char)3;
#else
          mystr[i - offset] = (char)255;
#endif
        } else if (IS_BIT_SET64(pattern_mask, i + offset)) {
          /*If the bit is set, there is a trigger at this position. We'd like to
           * display it as [] on screen*/
          /*Char 219 on the minicommand LCD is a []*/
#ifdef OLED_DISPLAY
          mystr[i] = (char)2;
#else
          mystr[i] = (char)219;
#endif
        }
      }
    }
  } else {

    for (int i = 0; i < mcl_seq.ext_tracks[last_ext_track].length; i++) {

      uint8_t step_count =
          ((MidiClock.div32th_counter /
            mcl_seq.ext_tracks[last_ext_track].resolution) -
           (mcl_actions_callbacks.start_clock32th /
            mcl_seq.ext_tracks[last_ext_track].resolution)) -
          (mcl_seq.ext_tracks[last_ext_track].length *
           ((MidiClock.div32th_counter /
                 mcl_seq.ext_tracks[last_ext_track].resolution -
             (mcl_actions_callbacks.start_clock32th /
              mcl_seq.ext_tracks[last_ext_track].resolution)) /
            (mcl_seq.ext_tracks[last_ext_track].length)));
      uint8_t noteson = 0;
      uint8_t notesoff = 0;

      for (uint8_t a = 0; a < 4; a++) {

        if (mcl_seq.ext_tracks[last_ext_track].notes[a][i] > 0) {

          noteson++;
          //    mystr[i] = (char) 219;
        }

        if (mcl_seq.ext_tracks[last_ext_track].notes[a][i] < 0) {
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
#ifdef OLED_DISPLAY
          mystr[i] = (char)0x5B;
#else
          mystr[i - offset] = (char)002;
#endif
        }
        note_held += noteson;
      } else if (notesoff > 0) {
        if ((i >= offset) && (i < offset + 16)) {
#ifdef OLED_DISPLAY
          mystr[i] = (char)0x5D;
#else
          mystr[i - offset] = (char)004;
#endif
        }
        note_held -= notesoff;
      } else {
        if (note_held >= 1) {
          if ((i >= offset) && (i < offset + 16)) {
#ifdef OLED_DISPLAY
            mystr[i] = (char)4;
#else
            mystr[i - offset] = (char)003;
#endif
          }
        }
      }

      if ((i >= mcl_seq.ext_tracks[last_ext_track].length) ||
          (step_count == i) && (MidiClock.state == 2)) {
        if ((i >= offset) && (i < offset + 16)) {
          mystr[i - offset] = ' ';
        }
      }
      if ((i >= offset) && (i < offset + 16)) {

        if (note_interface.notes[i - offset] == 1) {
          /*If the bit is set, there is a cue at this position. We'd like to
           * display it as [] on screen*/
          /*Char 219 on the minicommand LCD is a []*/
#ifdef OLED_DISPLAY
          mystr[i] = (char)3;
#else
          mystr[i - offset] = (char)255;
#endif
        }
      }
    }
  }

  /*Display the step sequencer pattern on screen, 16 steps at a time*/
  GUI.put_string_at(0, mystr);
}
void pattern_len_handler(Encoder *enc) {
  MCLEncoder *enc_ = (MCLEncoder *)enc;
  if (SeqPage::midi_device == DEVICE_MD) {
    DEBUG_PRINTLN("under 16");
    if (BUTTON_DOWN(Buttons.BUTTON3)) {
      for (uint8_t c = 0; c < 16; c++) {
        mcl_seq.md_tracks[c].length = enc_->cur;
      }
    } else {
      mcl_seq.md_tracks[last_md_track].length = enc_->cur;
    }
  } else {
    DEBUG_PRINTLN(enc_->cur);
    DEBUG_PRINTLN(enc_->max);
    DEBUG_PRINTLN(last_ext_track);
    if (BUTTON_DOWN(Buttons.BUTTON3)) {
      for (uint8_t c = 0; c < mcl_seq.num_ext_tracks; c++) {
        mcl_seq.ext_tracks[c].length = enc_->cur;
      }
    } else {
      mcl_seq.ext_tracks[last_ext_track].length = enc_->cur;
    }
  }
}
void SeqPage::display() {
  GUI.setLine(GUI.LINE1);
  GUI.put_value_at1(15, page_select + 1);
}

void SeqPageMidiEvents::setup_callbacks() {
  Midi.addOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&SeqPageMidiEvents::onControlChangeCallback_Midi);
}

void SeqPageMidiEvents::remove_callbacks() {
  Midi.removeOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&SeqPageMidiEvents::onControlChangeCallback_Midi);
}

void SeqPageMidiEvents::onControlChangeCallback_Midi(uint8_t *msg) {
  if (md_exploit.off()) {
    GUI.setPage(&grid_page);
    curpage = 0;
  }
}
