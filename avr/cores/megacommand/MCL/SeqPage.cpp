#include "SeqPage.h"
#include "MCL.h"

uint8_t SeqPage::page_select = 0;

uint8_t SeqPage::midi_device = DEVICE_MD;

uint8_t SeqPage::length = 0;
uint8_t SeqPage::resolution = 0;
uint8_t SeqPage::apply = 0;
uint8_t SeqPage::ignore_button_release = 255;
uint8_t SeqPage::page_count = 4;
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
  page_count = 4;
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
    if (track == md_exploit.track_with_nolocks) {
      md_exploit.off();
      note_interface.state = true;
      md_exploit.on();
    }
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
      page_select += 1;

      if (page_select >= page_count) {
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



void SeqPage::draw_lock_mask(uint8_t offset, uint64_t lock_mask, uint8_t step_count, uint8_t length, bool show_current_step) {
  mcl_gui.draw_leds(seq_x0, led_y, offset, lock_mask, step_count, length, show_current_step);
}

void SeqPage::draw_lock_mask(uint8_t offset, bool show_current_step) {
  auto &active_track = mcl_seq.md_tracks[last_md_track];
  draw_lock_mask(offset, active_track.lock_mask, active_track.step_count, active_track.length, show_current_step);
}

void SeqPage::draw_pattern_mask(uint8_t offset, uint64_t pattern_mask, uint8_t step_count, uint8_t length, bool show_current_step) {
  mcl_gui.draw_trigs(seq_x0, trig_y, offset, pattern_mask, step_count, length);
}

void SeqPage::draw_pattern_mask(uint8_t offset, uint8_t device,
                                bool show_current_step) {
  if (device == DEVICE_MD) {
    auto &active_track = mcl_seq.md_tracks[last_md_track];
    draw_pattern_mask(offset, active_track.pattern_mask, active_track.step_count, active_track.length, show_current_step);
  }
#ifdef EXT_TRACKS
  else {
    mcl_gui.draw_ext_track(seq_x0, trig_y, offset, last_ext_track, show_current_step);
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

void SeqPage::draw_page_index(bool show_page_index) {
  //  draw page index
  uint8_t pidx_x = pidx_x0;
  bool blink = MidiClock.getBlinkHint(true);
  // XXX should retrieve true track length
  uint8_t playing_idx = (MidiClock.bar_counter - 1) % page_count;
  uint8_t w = pidx_w;
  if (page_count == 8) { 
    w /= 2; 
    pidx_x -= 1;
  }

  for (uint8_t i = 0; i < page_count; ++i) {
    oled_display.drawRect(pidx_x, pidx_y, w, pidx_h, WHITE);

    // highlight page_select
    if ((page_select == i) && (show_page_index)) {
      oled_display.drawFastHLine(pidx_x + 1, pidx_y + 1, w - 2, WHITE);
    }

    // blink playing_idx
    if (playing_idx == i && blink) {
      if ((page_select == i) && (show_page_index)) {
        oled_display.drawFastHLine(pidx_x + 1, pidx_y + 1, w - 2, BLACK);
      } else {
        oled_display.drawFastHLine(pidx_x + 1, pidx_y + 1, w - 2, WHITE);
      }
    }

    pidx_x += w + 1;
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
  auto* oldfont = oled_display.getFont();
  oled_display.clearDisplay();

  bool is_md = (midi_device == DEVICE_MD);
#ifdef EXT_TRACKS
  bool ext_is_a4 = Analog4.connected;
#else
  bool ext_is_a4 = false;
#endif

  uint8_t track_id = last_md_track;
  if (!is_md) {
    track_id = last_ext_track;
  }
  track_id += 1;

  //  draw current active track
  mcl_gui.draw_panel_number(track_id);

  //  draw MD/EXT label
  const char* str_ext = "MI";
  if (ext_is_a4) {
    str_ext = "A4";
  }
  mcl_gui.draw_panel_toggle("MD", str_ext, is_md);

  //  draw stop/play/rec state
  mcl_gui.draw_panel_status(recording, MidiClock.state == 2);

  draw_page_index();
  //  draw info lines
  mcl_gui.draw_panel_labels(info1, info2);

  // if (show_track_menu) {
  // uint8_t x_offset = 43;
  // uint8_t y_offset = 8;
  // oled_display.setFont(&TomThumb);
  // oled_display.fillRect(84, 0, 40, 32, BLACK);
  // track_menu_page.draw_menu(86, y_offset, 39);
  //}
  oled_display.setFont(oldfont);
}
#endif

void SeqPage::draw_knob_frame() {
#ifndef OLED_DISPLAY
  return;
#endif
  mcl_gui.draw_knob_frame();
  // draw frame
}

void SeqPage::draw_knob(uint8_t i, const char *title, const char *text) {
  mcl_gui.draw_knob(i,title,text);
}

void SeqPage::draw_knob(uint8_t i, Encoder* enc, const char* title) {
  mcl_gui.draw_knob(i,enc,title);
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
