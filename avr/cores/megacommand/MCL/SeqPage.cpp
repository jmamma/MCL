#include "SeqPage.h"
#include "MCL.h"

uint8_t SeqPage::page_select = 0;

uint8_t SeqPage::midi_device = DEVICE_MD;

uint8_t SeqPage::length = 0;
uint8_t SeqPage::resolution = 0;
uint8_t SeqPage::apply = 0;
uint8_t SeqPage::ignore_button_release = 255;
bool SeqPage::show_track_menu = false;

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

void SeqPage::setup() { create_chars_seq(); }

void SeqPage::init() {
  ignore_button_release = 255;
  ((MCLEncoder *)encoders[2])->handler = pattern_len_handler;
  seqpage_midi_events.setup_callbacks();
#ifdef OLED_DISPLAY
  classic_display = false;
  oled_display.clearDisplay();
#endif
}

void SeqPage::cleanup() {
  seqpage_midi_events.remove_callbacks();
  note_interface.init_notes();
}

void SeqPage::select_track(uint8_t device, uint8_t track) {
  if (device == DEVICE_MD) {
    last_md_track = track;
#ifdef EXT_TRACK
    if (GUI.currentPage() == &seq_extstep_page) {
      GUI.setPage(&seq_step_page);
    }
#endif
    GUI.currentPage()->redisplay = true;
    GUI.currentPage()->config();
    encoders[2]->old = encoders[2]->cur;
  }
#ifdef EXT_TRACK
  else {
    last_ext_track = track - 16;
    if (GUI.currentPage() == &seq_step_page) {
      GUI.setPage(&seq_extstep_page);
    }
    GUI.currentPage()->redisplay = true;
    GUI.currentPage()->config();
    encoders[2]->old = encoders[2]->cur;
  }
#endif
}

bool SeqPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    uint8_t port = event->port;
    uint8_t device = midi_active_peering.get_device(port);
    uint8_t track = event->source - 128;

    //  TI + SHIFT1: adjust track seq length.
    //  Ignore SHIFT1 release event so it won't trigger
    //  a seq page select action.
    if (BUTTON_DOWN(Buttons.BUTTON2)) {
      //  calculate the intended seq length.
      uint8_t step = track;
      step += 1 + page_select * 16;
      //  Further, if SHIFT2 is pressed, set all tracks.
      if (SeqPage::midi_device == DEVICE_MD) {
        if (BUTTON_DOWN(Buttons.BUTTON3)) {
          for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {
            mcl_seq.md_tracks[n].length = step;
          }
        }
        mcl_seq.md_tracks[last_md_track].length = step;
      }
#ifdef EXT_TRACKS
      else {
        if (BUTTON_DOWN(Buttons.BUTTON3)) {
          for (uint8_t n = 0; n < NUM_EXT_TRACKS; n++) {
            mcl_seq.ext_tracks[n].length = step;
          }
        }
        mcl_seq.ext_tracks[last_ext_track].length = step;
      }
#endif
      encoders[2]->cur = step;
      if (event->mask == EVENT_BUTTON_RELEASED) {
        note_interface.notes[track] = 0;
        ignore_button_release = 2;
      }
      return true;
    }

    // TI + SHIFT2 = select track.
    if (BUTTON_DOWN(Buttons.BUTTON3)) {
      select_track(device, track);
      return true;
    }

    // notify derived class about unhandled TI event
    return false;
  } // end TI events

  // if no TI button pressed, enable page switching.
  if (note_interface.notes_all_off() || (note_interface.notes_count() == 0)) {
    if (EVENT_PRESSED(event, Buttons.ENCODER1)) {
      GUI.setPage(&seq_step_page);
      return false;
    }
    if (EVENT_PRESSED(event, Buttons.ENCODER2)) {
      GUI.setPage(&seq_rtrk_page);

      return false;
    }
    if (EVENT_PRESSED(event, Buttons.ENCODER3)) {

      GUI.setPage(&seq_param_page[0]);
      return false;
    }
    if (EVENT_PRESSED(event, Buttons.ENCODER4)) {

      GUI.setPage(&seq_ptc_page);

      return false;
    }
  }

  // A not-ignored BUTTON2 release event triggers sequence page select
  if (EVENT_RELEASED(event, Buttons.BUTTON2)) {
    if (ignore_button_release != 2) {
      ignore_button_release = 255;
      uint8_t pagemax = 4;
      page_select += 1;

      if (SeqPage::midi_device != DEVICE_MD) {
        pagemax = 8;
      }
      if (page_select >= pagemax) {
        page_select = 0;
      }
    }
    return false;
  }

  // SHIFT2 changes ENC4 to track select
  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
    encoders[3] = &trackselect_enc;
    if (midi_device == DEVICE_MD) {
      trackselect_enc.cur = trackselect_enc.old = last_md_track;
    }
#ifdef EXT_TRACKS
    else if (midi_device == DEVICE_A4) {
      trackselect_enc.cur = trackselect_enc.old = last_ext_track;
    }
#endif
  } else if (EVENT_RELEASED(event, Buttons.BUTTON3)) {
    encoders[3] = &seq_param4;
  }

  /*
  #ifdef OLED_DISPLAY

    if (EVENT_PRESSED(event, Buttons.BUTTON3)) {

      show_track_menu = true;
      if (midi_device == DEVICE_MD) {
        DEBUG_PRINTLN("okay using MD for length update");
        SeqPage::length = (mcl_seq.md_tracks[last_md_track].length);
        SeqPage::resolution = (mcl_seq.md_tracks[last_md_track].resolution);
      }
      if (midi_device == DEVICE_A4) {
        SeqPage::length = (mcl_seq.ext_tracks[last_ext_track].length);
        SeqPage::resolution = (mcl_seq.ext_tracks[last_ext_track].resolution);
      }

      encoders[0] = &track_menu_param1;
      encoders[1] = &track_menu_param2;
      track_menu_page.init();
      SeqPage::length = 0;
      return true;
    }
    if (EVENT_RELEASED(event, Buttons.BUTTON3)) {
      oled_display.clearDisplay();
      show_track_menu = false;
    void (*row_func)() =
  track_menu_page.menu.get_row_function(track_menu_page.encoders[1]->cur);
    DEBUG_PRINTLN(track_menu_page.encoders[1]->cur);
    if (row_func != NULL) {
            DEBUG_PRINTLN("func call");
        (*row_func)();
        return;
    }
      track_menu_page.enter();
      return true;
    }
  #else
    if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
      GUI.pushPage(&track_menu_page);
      return true;
    }
  #endif
  */
  return false;
}

#ifndef OLED_DISPLAY
void SeqPage::draw_lock_mask(uint8_t offset, bool show_current_step) {
  GUI.setLine(GUI.LINE2);

  auto &active_track = mcl_seq.md_tracks[last_md_track];
  char str[17] = "----------------";
  /*uint8_t step_count =
      (MidiClock.div16th_counter - mcl_actions.start_clock32th / 2) -
      (active_track.length *
       ((MidiClock.div16th_counter -
         mcl_actions.start_clock32th / 2) /
        active_track.length)); */
  uint8_t step_count = active_track.step_count;
  for (int i = 0; i < 16; i++) {

    if (i + offset >= active_track.length) {
      str[i] = ' ';
    } else if ((show_current_step) && (step_count == i + offset) &&
               (MidiClock.state == 2)) {
      str[i] = ' ';
    } else {
      if (IS_BIT_SET64(active_track.lock_mask, i + offset)) {
        str[i] = 'x';
      }
      if (IS_BIT_SET64(active_track.pattern_mask, i + offset) &&
          !IS_BIT_SET64(active_track.lock_mask, i + offset)) {
        str[i] = (char)165;
      }
      if (IS_BIT_SET64(active_track.pattern_mask, i + offset) &&
          IS_BIT_SET64(active_track.lock_mask, i + offset)) {
        str[i] = (char)219;
      }

      if (note_interface.notes[i] == 1) {
        /*Char 219 on the minicommand LCD is a []*/
        str[i] = (char)255;
      }
    }
  }
  GUI.put_string_at(0, str);
}

void SeqPage::draw_pattern_mask(uint8_t offset, uint8_t device,
                                bool show_current_step) {
  GUI.setLine(GUI.LINE2);

  char mystr[17] = "                ";

  auto &active_track = mcl_seq.md_tracks[last_md_track];
  uint64_t pattern_mask = active_track.pattern_mask;
  int8_t note_held = 0;

  if (device == DEVICE_MD) {

    for (int i = 0; i < 16; i++) {
      if (device == DEVICE_MD) {
        // uint32_t new_count = MidiClock.div96th_counter;
#ifdef OLED_DISPLAY
        uint32_t count_16th = (MidiClock.div96th_counter + 9) / 6;
#else
        uint32_t count_16th = MidiClock.div96th_counter / 6;
#endif
        /*    uint8_t step_count = (count_16th -
                                  mcl_actions_callbacks.start_clock96th / 5) -
                                 (active_track.length *
                                  ((count_16th -
                                    mcl_actions_callbacks.start_clock96th / 5) /
                                   active_track.length));*/
        /* uint8_t step_count = (MidiClock.div16th_counter -
                               mcl_actions.start_clock32th / 2) -
                              (active_track.length *
                               ((MidiClock.div16th_counter -
                                 mcl_actions.start_clock32th / 2) /
                                active_track.length)); */

        uint8_t step_count = active_track.step_count;
#ifdef OLED_DISPLAY
#endif
        if (i + offset >= active_track.length) {
          mystr[i] = ' ';
        } else if ((show_current_step) && (step_count == i + offset) &&
                   (MidiClock.state == 2)) {
          mystr[i] = ' ';
        } else if (note_interface.notes[i] == 1) {
          /*Char 219 on the minicommand LCD is a []*/
#ifdef OLED_DISPLAY
          mystr[i] = (char)3;
#else
          mystr[i] = (char)255;
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
        } else {
          mystr[i] = '-';
        }
      }
    }
  }
#ifdef EXT_TRACKS
  else {

    for (int i = 0; i < mcl_seq.ext_tracks[last_ext_track].length; i++) {

      /* uint8_t step_count =
           ((MidiClock.div32th_counter /
             mcl_seq.ext_tracks[last_ext_track].resolution) -
            (mcl_actions.start_clock32th /
             mcl_seq.ext_tracks[last_ext_track].resolution)) -
           (mcl_seq.ext_tracks[last_ext_track].length *
            ((MidiClock.div32th_counter /
                  mcl_seq.ext_tracks[last_ext_track].resolution -
              (mcl_actions.start_clock32th /
               mcl_seq.ext_tracks[last_ext_track].resolution)) /
             (mcl_seq.ext_tracks[last_ext_track].length)));
       */
      uint8_t step_count = mcl_seq.ext_tracks[last_ext_track].step_count;
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
      if ((i >= offset) && (i < offset + 16)) {
        mystr[i - offset] = '-';
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
          mystr[i - offset] = (char)0x5B;
#else
          mystr[i - offset] = (char)002;
#endif
        }
        note_held += noteson;
      } else if (notesoff > 0) {
        if ((i >= offset) && (i < offset + 16)) {
#ifdef OLED_DISPLAY
          mystr[i - offset] = (char)0x5D;
#else
          mystr[i - offset] = (char)004;
#endif
        }
        note_held -= notesoff;
      } else {
        if (note_held >= 1) {
          if ((i >= offset) && (i < offset + 16)) {
#ifdef OLED_DISPLAY
            mystr[i - offset] = (char)4;
#else
            mystr[i - offset] = (char)003;
#endif
          }
        }
      }

      if ((step_count == i) && (MidiClock.state == 2)) {
        if ((i >= offset) && (i < offset + 16)) {
          mystr[i - offset] = ' ';
        }
      }
      if ((i >= offset) && (i < offset + 16)) {

        if (note_interface.notes[i - offset] == 1) {
#ifdef OLED_DISPLAY
          mystr[i - offset] = (char)3;
#else
          mystr[i - offset] = (char)255;
#endif
        }
      }
    }
  }
#endif

  /*Display the step sequencer pattern on screen, 16 steps at a time*/
  GUI.put_string_at(0, mystr);
}
#else
void SeqPage::draw_lock_mask(uint8_t offset, bool show_current_step) {
  auto &active_track = mcl_seq.md_tracks[last_md_track];
  uint8_t step_count = active_track.step_count;

  uint8_t led_x = seq_x0;

  for (int i = 0; i < 16; i++) {

    uint8_t idx = i + offset;
    bool in_range = idx < active_track.length;
    bool current =
        show_current_step && step_count == idx && MidiClock.state == 2;
    bool locked = in_range && IS_BIT_SET64(active_track.lock_mask, i + offset);

    if (note_interface.notes[i] == 1) {
      // TI feedback
      oled_display.fillRect(led_x - 1, led_y - 1, seq_w + 2, led_h + 1, WHITE);
    } else if (!in_range) {
      // don't draw
    } else if (current ^ locked) {
      // highlight
      oled_display.fillRect(led_x, led_y, seq_w, led_h, WHITE);
    } else if (current && locked) {
      // highlight 2
      oled_display.fillRect(led_x, led_y, seq_w, led_h, WHITE);
      oled_display.drawPixel(led_x + 2, led_y + 1, BLACK);
    } else {
      // frame only
      oled_display.drawRect(led_x, led_y, seq_w, led_h, WHITE);
    }

    led_x += seq_w + 1;
  }
}

void SeqPage::draw_pattern_mask(uint8_t offset, uint8_t device,
                                bool show_current_step) {

  uint8_t trig_x = seq_x0;

  if (device == DEVICE_MD) {
    auto &active_track = mcl_seq.md_tracks[last_md_track];
    uint64_t pattern_mask = active_track.pattern_mask;

    for (int i = 0; i < 16; i++) {

      uint8_t idx = i + offset;
      bool in_range = idx < active_track.length;

      if (note_interface.notes[i] == 1) {
        // TI feedback
        oled_display.fillRect(trig_x - 1, trig_y, seq_w + 2, trig_h + 1, WHITE);
      } else if (!in_range) {
        // don't draw
      } else {
        if (IS_BIT_SET64(pattern_mask, i + offset)) {
          /*If the bit is set, there is a trigger at this position. */
          oled_display.fillRect(trig_x, trig_y, seq_w, trig_h, WHITE);
        } else {
          oled_display.drawRect(trig_x, trig_y, seq_w, trig_h, WHITE);
        }
      }

      trig_x += seq_w + 1;
    }
  }
#ifdef EXT_TRACKS
  else {

    int8_t note_held = 0;
    auto &active_track = mcl_seq.ext_tracks[last_ext_track];
    for (int i = 0; i < active_track.length; i++) {

      uint8_t step_count = active_track.step_count;
      uint8_t noteson = 0;
      uint8_t notesoff = 0;
      bool in_range = (i >= offset) && (i < offset + 16);
      bool right_most = (i == active_track.length - 1);

      for (uint8_t a = 0; a < 4; a++) {
        if (active_track.notes[a][i] > 0) {
          noteson++;
        }
        if (active_track.notes[a][i] < 0) {
          notesoff++;
        }
      }

      note_held += noteson;
      note_held -= notesoff;

      if (!in_range) {
        continue;
      }

      if (note_interface.notes[i - offset] == 1) {
        oled_display.fillRect(trig_x, trig_y, seq_w, trig_h, WHITE);
      } else if (!note_held) { // --
        oled_display.drawFastHLine(trig_x - 1, trig_y + 2, seq_w + 2, WHITE);
      } else { // draw top, bottom
        oled_display.drawFastHLine(trig_x - 1, trig_y, seq_w + 2, WHITE);
        oled_display.drawFastHLine(trig_x - 1, trig_y + trig_h - 1, seq_w + 2,
                                   WHITE);
      }

      if (noteson > 0 || notesoff > 0) { // left |
        oled_display.drawFastVLine(trig_x - 1, trig_y, trig_h, WHITE);
      }

      if (right_most) { // right |
        oled_display.drawFastVLine(trig_x + seq_w, trig_y, trig_h, WHITE);
      }

      if ((step_count == i) && (MidiClock.state == 2) && show_current_step) {
        oled_display.fillRect(trig_x, trig_y, seq_w, trig_h, INVERT);
      }

      trig_x += seq_w + 1;
    }
  }
#endif
}

#endif // OLED_DISPLAY

void pattern_len_handler(Encoder *enc) {
  MCLEncoder *enc_ = (MCLEncoder *)enc;
  if (!enc_->hasChanged()) {
    return;
  }
  if (SeqPage::midi_device == DEVICE_MD) {
    DEBUG_PRINTLN("under 16");
    if (BUTTON_DOWN(Buttons.BUTTON3)) {
      for (uint8_t c = 0; c < 16; c++) {
        mcl_seq.md_tracks[c].set_length(enc_->cur);
      }
    } else {
      mcl_seq.md_tracks[last_md_track].set_length(enc_->cur);
    }
  }
#ifdef EXT_TRACKS
  else {
    if (BUTTON_DOWN(Buttons.BUTTON3)) {
      for (uint8_t c = 0; c < mcl_seq.num_ext_tracks; c++) {
        mcl_seq.ext_tracks[c].buffer_notesoff();
        mcl_seq.ext_tracks[c].set_length(enc_->cur);
      }
    } else {
      mcl_seq.ext_tracks[last_ext_track].buffer_notesoff();
      mcl_seq.ext_tracks[last_ext_track].set_length(enc_->cur);
    }
  }
#endif
}

void SeqPage::loop() {

  if (show_track_menu) {
    if (midi_device == DEVICE_MD) {
      DEBUG_PRINTLN("okay using MD for length update");
      (mcl_seq.md_tracks[last_md_track].length) = SeqPage::length;
      (mcl_seq.md_tracks[last_md_track].resolution) = SeqPage::resolution;
    }
#ifdef EXT_TRACKS
    if (midi_device == DEVICE_A4) {
      (mcl_seq.ext_tracks[last_ext_track].length) = SeqPage::length;
      (mcl_seq.ext_tracks[last_ext_track].resolution) = SeqPage::resolution;
    }
#endif
    track_menu_page.loop();
    return;
  }

  if (trackselect_enc.hasChanged()) {

    auto plus = trackselect_enc.cur > trackselect_enc.old;
    auto track = last_md_track;
#ifdef EXT_TRACKS
    if (midi_device == DEVICE_A4) {
      track = last_ext_track;
    }
#endif
    if (plus && track < 15) {
      ++track;
    } else if (!plus && track > 0) {
      --track;
    }
    select_track(midi_device, track);
    trackselect_enc.old = trackselect_enc.cur = track;
  }
}

#ifndef OLED_DISPLAY
void SeqPage::display() {
  for (uint8_t i = 0; i < 2; i++) {
    for (int j = 0; j < 16; j++) {
      if (GUI.lines[i].data[j] == 0) {
        GUI.lines[i].data[j] = ' ';
      }
    }
    LCD.goLine(i);
    LCD.puts(GUI.lines[i].data);
    GUI.lines[i].changed = false;
  }
  GUI.setLine(GUI.LINE1);
}
#else
//  ref: design/Sequencer.png
void SeqPage::display() {
  oled_display.clearDisplay();

  bool is_md = (midi_device == DEVICE_MD);
#ifdef EXT_TRACKS
  bool ext_is_a4 = (seq_extstep_page.midi_device == DEVICE_A4);
#else
  bool ext_is_a4 = false;
#endif

  uint8_t track_id = last_md_track;
  if (!is_md) {
    track_id = last_ext_track;
  }
  track_id += 1;

  //  draw current active track
  oled_display.setTextColor(WHITE);
  oled_display.setFont(&Elektrothic);
  oled_display.setCursor(trackid_x, trackid_y);
  if (track_id < 10) {
    oled_display.print('0');
  }
  oled_display.print(track_id);

  oled_display.setFont(&TomThumb);
  //  draw MD/EXT label
  if (is_md) {
    oled_display.fillRect(label_x, label_md_y, label_w, label_h, WHITE);
    oled_display.setCursor(label_x + 1, label_md_y + 6);
    oled_display.setTextColor(BLACK);
    oled_display.print("MD");
    oled_display.setTextColor(WHITE);
  } else {
    oled_display.setCursor(label_x + 1, label_md_y + 6);
    oled_display.setTextColor(WHITE);
    oled_display.print("MD");
    oled_display.fillRect(label_x, label_ex_y, label_w, label_h, WHITE);
    oled_display.setTextColor(BLACK);
  }
  oled_display.setCursor(label_x + 1, label_ex_y + 6);
  if (ext_is_a4) {
    oled_display.print("A4");
  } else {
    oled_display.print("MI");
  }

  //  draw stop/play/rec state
  if (recording) {
    oled_display.fillRect(cir_x1, tri_y, 4, 5, WHITE);
    oled_display.drawPixel(cir_x1, tri_y, BLACK);
    oled_display.drawPixel(cir_x2, tri_y, BLACK);
    oled_display.drawPixel(cir_x1, tri_y + 4, BLACK);
    oled_display.drawPixel(cir_x2, tri_y + 4, BLACK);
  } else if (MidiClock.state == 2) {
    oled_display.drawLine(tri_x, tri_y, tri_x, tri_y + 4, WHITE);
    oled_display.fillTriangle(tri_x + 1, tri_y, tri_x + 3, tri_y + 2, tri_x + 1,
                              tri_y + 4, WHITE);
  } else {
    oled_display.fillRect(tri_x, tri_y, 4, 5, WHITE);
  }

  //  draw page index
  uint8_t pidx_x = pidx_x0;
  bool blink = !MidiClock.getBlinkHint(true);
  uint8_t playing_idx = (MidiClock.bar_counter - 1) % 4;
  for (uint8_t i = 0; i < 4; ++i) {
    oled_display.drawRect(pidx_x, pidx_y, pidx_w, pidx_h, WHITE);

    // highlight page_select
    if (page_select == i) {
      oled_display.drawFastHLine(pidx_x + 1, pidx_y + 1, pidx_w - 2, WHITE);
    }

    // blink playing_idx
    if (playing_idx == i && blink) {
      if (page_select == i) {
        oled_display.drawFastHLine(pidx_x + 2, pidx_y + 1, 2, BLACK);
      } else {
        oled_display.drawFastHLine(pidx_x + 1, pidx_y + 1, pidx_w - 2, WHITE);
      }
    }

    pidx_x += pidx_w + 1;
  }

  //  draw info lines
  oled_display.fillRect(0, info1_y, pane_w, info_h, WHITE);
  oled_display.setTextColor(BLACK);
  oled_display.setCursor(1, info1_y + 6);
  oled_display.print(info1);
  oled_display.setTextColor(WHITE);
  oled_display.setCursor(1, info2_y + 6);
  oled_display.print(info2);

  // if (show_track_menu) {
  // uint8_t x_offset = 43;
  // uint8_t y_offset = 8;
  // oled_display.setFont(&TomThumb);
  // oled_display.fillRect(84, 0, 40, 32, BLACK);
  // track_menu_page.draw_menu(86, y_offset, 39);
  //}
}
#endif

void SeqPage::draw_knob_frame() {
#ifndef OLED_DISPLAY
  return;
#endif
  // draw frame
  for (uint8_t x = knob_x0; x <= knob_xend; x += knob_w) {
    mcl_gui.draw_vertical_dashline(x, 0, knob_y);
    oled_display.drawPixel(x, knob_y, WHITE);
    oled_display.drawPixel(x, knob_y + 1, WHITE);
  }
  mcl_gui.draw_horizontal_dashline(knob_y, knob_x0 + 1, knob_xend + 1);
}

void SeqPage::draw_knob(uint8_t i, const char *title, const char *text) {
  uint8_t x = knob_x0 + i * knob_w;
  oled_display.setFont(&TomThumb);
  oled_display.setTextColor(WHITE);
  oled_display.setCursor(x + 4, 7);
  oled_display.print(title);

  oled_display.setFont();
  oled_display.setCursor(x + 4, 9);
  oled_display.print(text);
}

void SeqPage::draw_knob(uint8_t i, Encoder* enc, const char* title) {
  uint8_t x = knob_x0 + i * knob_w;
  mcl_gui.draw_light_encoder(x + 6, 6, enc, title);
}

void SeqPageMidiEvents::setup_callbacks() {
  //   Midi.addOnControlChangeCallback(
  //      this,
  //      (midi_callback_ptr_t)&SeqPageMidiEvents::onControlChangeCallback_Midi);
}

void SeqPageMidiEvents::remove_callbacks() {
  //  Midi.removeOnControlChangeCallback(
  //      this,
  //    (midi_callback_ptr_t)&SeqPageMidiEvents::onControlChangeCallback_Midi);
}

void SeqPageMidiEvents::onControlChangeCallback_Midi(uint8_t *msg) {}
