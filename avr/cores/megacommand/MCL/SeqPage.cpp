#include "MCL.h"
#include "SeqPage.h"

uint8_t SeqPage::page_select = 0;

uint8_t SeqPage::midi_device = DEVICE_MD;

uint8_t SeqPage::page_count = 4;
bool SeqPage::show_seq_menu = false;
bool SeqPage::show_step_menu = false;
bool SeqPage::toggle_device = true;

uint8_t SeqPage::step_select = 255;

uint8_t opt_scale = 1;
uint8_t opt_trackid = 1;
uint8_t opt_copy = 0;
uint8_t opt_paste = 0;
uint8_t opt_clear = 0;
uint8_t opt_shift = 0;
uint8_t opt_reverse = 0;
uint8_t opt_clear_step = 0;

static uint8_t opt_midi_device_capture = DEVICE_MD;
static SeqPage *opt_seqpage_capture = nullptr;
static MCLEncoder *opt_param1_capture = nullptr;
static MCLEncoder *opt_param2_capture = nullptr;

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
  if (mcl_cfg.track_select == 1) {
    md_track_select.on();
  }
  page_count = 4;
  ((MCLEncoder *)encoders[2])->handler = pattern_len_handler;
  seqpage_midi_events.setup_callbacks();
#ifdef OLED_DISPLAY
  classic_display = false;
  oled_display.clearDisplay();
#endif
  toggle_device = true;
  seq_menu_page.menu.enable_entry(0, false);
  seq_menu_page.menu.enable_entry(1, false);
  if (mcl_cfg.track_select == 1) {
  seq_menu_page.menu.enable_entry(2, false);
  }
  else {
  seq_menu_page.menu.enable_entry(2, true);
  }
}

void SeqPage::cleanup() {
  if (mcl_cfg.track_select == 1) {
    md_track_select.off();
  }
  seqpage_midi_events.remove_callbacks();
  note_interface.init_notes();
}

void SeqPage::select_track(uint8_t device, uint8_t track) {
  if (device == DEVICE_MD) {

    last_md_track = track;
  }
#ifdef EXT_TRACKS
  else {
    last_ext_track = min(track, 3); // XXX
  }
#endif
  GUI.currentPage()->redisplay = true;
  GUI.currentPage()->config();
}

bool SeqPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    uint8_t port = event->port;
    uint8_t device = midi_active_peering.get_device(port);
    uint8_t track = event->source - 128;

    // =================== seq menu mode TI events ================

    if (show_seq_menu) {
      // TI + SHIFT2 = select track.
      if (BUTTON_DOWN(Buttons.BUTTON3) && (mcl_cfg.track_select == 0)) {
        opt_trackid = track + 1;
        note_interface.ignoreNextEvent(track);
        select_track(device, track);
        redisplay = true;
        seq_menu_page.select_item(0);
      }

      return true;
    }

    // =================== normal mode TI events ================

    //  TI + WRITE (BUTTON4): adjust track seq length.
    //  Ignore WRITE release event so it won't trigger
    //  a page select action.
    if (BUTTON_DOWN(Buttons.BUTTON4)) {
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
      note_interface.ignoreNextEvent(track);
      if (event->mask == EVENT_BUTTON_RELEASED) {
        note_interface.notes[track] = 0;
      }
      GUI.ignoreNextEvent(Buttons.BUTTON4);
      if (BUTTON_DOWN(Buttons.BUTTON3)) {
        GUI.ignoreNextEvent(Buttons.BUTTON3);
      }
      return true;
    }

    // notify derived class about unhandled TI event
    return false;
  } // end TI events

  // A not-ignored WRITE (BUTTON4) release event triggers sequence page select
  if (EVENT_RELEASED(event, Buttons.BUTTON4)) {
    page_select += 1;

    if (page_select >= page_count) {
      page_select = 0;
    }
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    GUI.setPage(&page_select_page);
  }

#ifdef OLED_DISPLAY
  // activate show_seq_menu only if S2 press is not a key combination
  if (EVENT_PRESSED(event, Buttons.BUTTON3) && !BUTTON_DOWN(Buttons.BUTTON4)) {
    // If MD trig is held and BUTTON3 is pressed, launch note menu
    if ((note_interface.notes_count_on() != 0) && (!show_step_menu) &&
        (GUI.currentPage() != &seq_ptc_page)) {
      uint8_t note = 255;
      for (uint8_t n = 0; n < NUM_MD_TRACKS && note == 255; n++) {
        if (note_interface.notes[n] == 1) {
          note = n;
        }
      }
      if (note == 255) {
        return false;
      }
      step_select = note;

      opt_param1_capture = (MCLEncoder *)encoders[0];
      opt_param2_capture = (MCLEncoder *)encoders[1];
      encoders[0] = &step_menu_value_encoder;
      encoders[1] = &step_menu_entry_encoder;
      step_menu_page.init();
      show_step_menu = true;
      return true;
    } else if (!show_seq_menu) {
      show_seq_menu = true;
      // capture current midi_device value
      opt_midi_device_capture = midi_device;
      // capture current page.
      opt_seqpage_capture = this;

      if (opt_midi_device_capture == DEVICE_MD) {
        DEBUG_PRINTLN("okay using MD for length update");
        opt_trackid = last_md_track + 1;
        opt_scale = (mcl_seq.md_tracks[last_md_track].scale);
      } else {
      #ifdef EXT_TRACKS
        opt_trackid = last_ext_track + 1;
        opt_scale = (mcl_seq.ext_tracks[last_ext_track].scale);
      #endif
      }

      opt_param1_capture = (MCLEncoder *)encoders[0];
      opt_param2_capture = (MCLEncoder *)encoders[1];
      encoders[0] = &seq_menu_value_encoder;
      encoders[1] = &seq_menu_entry_encoder;
      seq_menu_page.init();
      return true;
    }
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON3)) {
    encoders[0] = opt_param1_capture;
    encoders[1] = opt_param2_capture;
    oled_display.clearDisplay();
    void (*row_func)();
    if (show_seq_menu) {
      void (*row_func)() =
          seq_menu_page.menu.get_row_function(seq_menu_page.encoders[1]->cur);

    } else if (show_step_menu) {
      void (*row_func)() =
          step_menu_page.menu.get_row_function(step_menu_page.encoders[1]->cur);
    }
    if (row_func != NULL) {
      (*row_func)();
      show_seq_menu = false;
      show_step_menu = false;
      return true;
    }
    if (show_seq_menu) {
      seq_menu_page.enter();
    }
    if (show_step_menu) {
      step_menu_page.enter();
    }

    show_seq_menu = false;
    show_step_menu = false;
    mcl_gui.init_encoders_used_clock();
    return true;
  }
#else
  if (EVENT_PRESSED(event, Buttons.BUTTON3) && !BUTTON_DOWN(Buttons.BUTTON4)) {
    // If MD trig is held and BUTTON3 is pressed, launch note menu
    if ((note_interface.notes_count_on() != 0) &&
        (GUI.currentPage() != &seq_ptc_page)) {
      uint8_t note = 255;
      for (uint8_t n = 0; n < NUM_MD_TRACKS && note == 255; n++) {
        if (note_interface.notes[n] == 1) {
          note = n;
        }
      }
      if (note == 255) {
        return false;
      }
      step_select = note;
      show_step_menu = true;
      GUI.pushPage(&step_menu_page);
    } else {
      if (opt_midi_device_capture == DEVICE_MD) {
        DEBUG_PRINTLN("okay using MD for length update");
        opt_trackid = last_md_track + 1;
        opt_scale = (mcl_seq.md_tracks[last_md_track].scale);
      } else {
      #ifdef EXT_TRACKS
        opt_trackid = last_ext_track + 1;
        opt_scale = (mcl_seq.ext_tracks[last_ext_track].scale);
      #endif
      }
      // capture current midi_device value
      opt_midi_device_capture = midi_device;
      // capture current page.
      opt_seqpage_capture = this;
      GUI.pushPage(&seq_menu_page);
      show_seq_menu = true;
      return true;
    }
  }
#endif

  // legacy enc push page switching code
  /*
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
             mcl_seq.ext_tracks[last_ext_track].scale) -
            (mcl_actions.start_clock32th /
             mcl_seq.ext_tracks[last_ext_track].scale)) -
           (mcl_seq.ext_tracks[last_ext_track].length *
            ((MidiClock.div32th_counter /
                  mcl_seq.ext_tracks[last_ext_track].scale -
              (mcl_actions.start_clock32th /
               mcl_seq.ext_tracks[last_ext_track].scale)) /
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

void SeqPage::draw_lock_mask(uint8_t offset, uint64_t lock_mask,
                             uint8_t step_count, uint8_t length,
                             bool show_current_step) {
  mcl_gui.draw_leds(MCLGUI::seq_x0, MCLGUI::led_y, offset, lock_mask,
                    step_count, length, show_current_step);
}

void SeqPage::draw_lock_mask(uint8_t offset, bool show_current_step) {
  auto &active_track = mcl_seq.md_tracks[last_md_track];
  draw_lock_mask(offset, active_track.lock_mask, active_track.step_count,
                 active_track.length, show_current_step);
}

void SeqPage::draw_pattern_mask(uint8_t offset, uint64_t pattern_mask,
                                uint8_t step_count, uint8_t length,
                                bool show_current_step, uint64_t mute_mask) {
  mcl_gui.draw_trigs(MCLGUI::seq_x0, MCLGUI::trig_y, offset, pattern_mask,
                     step_count, length, mute_mask);
}

void SeqPage::draw_pattern_mask(uint8_t offset, uint8_t device,
                                bool show_current_step) {
  if (device == DEVICE_MD) {
    auto &active_track = mcl_seq.md_tracks[last_md_track];
    draw_pattern_mask(offset, active_track.pattern_mask,
                      active_track.step_count, active_track.length,
                      show_current_step, active_track.oneshot_mask);
  }
#ifdef EXT_TRACKS
  else {
    mcl_gui.draw_ext_track(MCLGUI::seq_x0, MCLGUI::trig_y, offset,
                           last_ext_track, show_current_step);
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
    if (BUTTON_DOWN(Buttons.BUTTON4)) {
      for (uint8_t c = 0; c < 16; c++) {
        mcl_seq.md_tracks[c].set_length(enc_->cur);
      }
    } else {
      mcl_seq.md_tracks[last_md_track].set_length(enc_->cur);
    }
  }
#ifdef EXT_TRACKS
  else {
    if (BUTTON_DOWN(Buttons.BUTTON4)) {
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

void opt_trackid_handler() {
  opt_seqpage_capture->select_track(opt_midi_device_capture, opt_trackid - 1);
}

void opt_scale_handler() {

  if (opt_midi_device_capture == DEVICE_MD) {
    DEBUG_PRINTLN("okay using MD for length update");
    (mcl_seq.md_tracks[last_md_track].set_scale(opt_scale);
     seq_step_page.config_encoders(opt_scale);
  }
#ifdef EXT_TRACKS
  else {
    (mcl_seq.ext_tracks[last_ext_track].set_scale(opt_scale);
    seq_extstep_page.config_encoders(opt_scale);
  }
#endif
  opt_seqpage_capture->init();
}

void opt_clear_track_handler() {
  if (opt_midi_device_capture == DEVICE_MD) {
    if (opt_clear == 2) {
#ifdef OLED_DISPLAY
      oled_display.textbox("CLEAR MD ", "TRACKS");
#endif

      for (uint8_t n = 0; n < 16; ++n) {
        mcl_seq.md_tracks[n].clear_track();
      }
    } else if (opt_clear == 1) {
#ifdef OLED_DISPLAY
      oled_display.textbox("CLEAR TRACK", "");
#endif

      mcl_seq.md_tracks[last_md_track].clear_track();
    }
  }
#ifdef EXT_TRACKS
  else {
    if (opt_clear == 2) {
      for (uint8_t n = 0; n < mcl_seq.num_ext_tracks; n++) {
#ifdef OLED_DISPLAY
        oled_display.textbox("CLEAR EXT ", "TRACKS");
#endif
        mcl_seq.ext_tracks[n].clear_track();
      }
    } else if (opt_clear == 1) {
#ifdef OLED_DISPLAY
      oled_display.textbox("CLEAR EXT ", "TRACK");
#endif
      mcl_seq.ext_tracks[last_ext_track].clear_track();
    }
  }
#endif
  opt_clear = 0;
}

void opt_clear_locks_handler() {
  if (opt_midi_device_capture == DEVICE_MD) {
    if (opt_clear == 2) {
      for (uint8_t n = 0; n < 16; ++n) {
#ifdef OLED_DISPLAY
        oled_display.textbox("CLEAR MD ", "LOCKS");
#endif

        mcl_seq.md_tracks[n].clear_locks();
      }
    } else if (opt_clear == 1) {
#ifdef OLED_DISPLAY
      oled_display.textbox("CLEAR ", "LOCKS");
#endif
      mcl_seq.md_tracks[last_md_track].clear_locks();
    }
  } else {
    // TODO ext locks
  }
  opt_clear = 0;
}

void opt_clear_all_tracks_handler() {
  if (opt_midi_device_capture == DEVICE_MD) {
  }
#ifdef EXT_TRACKS
  else {
    mcl_seq.ext_tracks[last_ext_track].clear_track();
  }
#endif
}

void opt_clear_all_locks_handler() {
  if (opt_midi_device_capture == DEVICE_MD) {
  }
#ifdef EXT_TRACKS
  else {
    // TODO ext locks
  }
#endif
}

void opt_copy_track_handler() {
  if (opt_copy == 2) {

    if (opt_midi_device_capture == DEVICE_MD) {
#ifdef OLED_DISPLAY
      oled_display.textbox("COPY MD ", "TRACKS");
#endif

      mcl_clipboard.copy_sequencer();
    }
#ifdef EXT_TRACKS
    else {
#ifdef OLED_DISPLAY
      oled_display.textbox("COPY EXT ", "TRACKS");
#endif
      mcl_clipboard.copy_sequencer(NUM_MD_TRACKS);
    }
#endif
  }
  if (opt_copy == 1) {
    if (opt_midi_device_capture == DEVICE_MD) {
#ifdef OLED_DISPLAY
      oled_display.textbox("COPY TRACK", "");
#endif
      mcl_clipboard.copy_track = last_md_track;
      mcl_clipboard.copy_sequencer_track(last_md_track);
    }
#ifdef EXT_TRACKS
    else {
#ifdef OLED_DISPLAY
      oled_display.textbox("COPY EXT ", "TRACK");
#endif

      mcl_clipboard.copy_track = last_ext_track + NUM_MD_TRACKS;
      mcl_clipboard.copy_sequencer_track(last_ext_track + NUM_MD_TRACKS);
    }
#endif
  }
  opt_copy = 0;
}

void opt_paste_track_handler() {
  if (opt_paste == 2) {

    if (opt_midi_device_capture == DEVICE_MD) {
#ifdef OLED_DISPLAY
      oled_display.textbox("PASTE MD ", "TRACKS");
#endif
      mcl_clipboard.paste_sequencer();
    }
#ifdef EXT_TRACKS
    else {
#ifdef OLED_DISPLAY
      oled_display.textbox("PASTE EXT ", "TRACKS");
#endif
      mcl_clipboard.paste_sequencer(NUM_MD_TRACKS);
    }
#endif
  }
  if (opt_paste == 1) {
    if (opt_midi_device_capture == DEVICE_MD) {
#ifdef OLED_DISPLAY
      oled_display.textbox("PASTE TRACK", "");
#endif
      mcl_clipboard.paste_sequencer_track(mcl_clipboard.copy_track,
                                          last_md_track);
    }
#ifdef EXT_TRACKS
    else {
#ifdef OLED_DISPLAY
      oled_display.textbox("PASTE EXT ", "TRACK");
#endif
      mcl_clipboard.paste_sequencer_track(mcl_clipboard.copy_track,
                                          last_ext_track + NUM_MD_TRACKS);
    }
#endif
  }
  opt_paste = 0;
}

void opt_copy_step_handler() {
#ifdef OLED_DISPLAY
  oled_display.textbox("COPY STEP", "");
#endif
  mcl_seq.md_tracks[last_md_track].copy_step(
      SeqPage::step_select + SeqPage::page_select * 16, &mcl_clipboard.step);
}

void opt_paste_step_handler() {
#ifdef OLED_DISPLAY
  oled_display.textbox("PASTE STEP", "");
#endif
  mcl_seq.md_tracks[last_md_track].paste_step(
      SeqPage::step_select + SeqPage::page_select * 16, &mcl_clipboard.step);
}

void opt_mute_step_handler() {
  for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {
    if (note_interface.notes[n] == 1) {
      TOGGLE_BIT64(mcl_seq.md_tracks[last_md_track].oneshot_mask,
                   n + SeqPage::page_select * 16);
    }
  }
}

void opt_clear_step_locks_handler() {
 if (opt_clear_step == 1) {
#ifdef OLED_DISPLAY
  oled_display.textbox("CLEAR STEP: ", "LOCKS");
#endif
    for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {
      if (note_interface.notes[n] == 1) {

        if (opt_midi_device_capture == DEVICE_MD) {
          mcl_seq.md_tracks[last_md_track].clear_step_locks(
              n + SeqPage::page_select * 16);
        } else {
          //        mcl_seq.ext_tracks[last_ext_track].clear_step_locks(
          //          SeqPage::step_select + SeqPage::page_select * 16);
        }
      }
    }
  }
  opt_clear_step = 0;
}

void opt_shift_track_handler() {
  switch (opt_shift) {
  case 1:
    if (opt_midi_device_capture == DEVICE_MD) {
      mcl_seq.md_tracks[last_md_track].rotate_left();
    }
#ifdef EXT_TRACKS
    else {
      mcl_seq.ext_tracks[last_ext_track].rotate_left();
    }
#endif
    break;
  case 2:
    if (opt_midi_device_capture == DEVICE_MD) {
      mcl_seq.md_tracks[last_md_track].rotate_right();
    }
#ifdef EXT_TRACKS
    else {
      mcl_seq.ext_tracks[last_ext_track].rotate_right();
    }
#endif
    break;
  case 3:
    if (opt_midi_device_capture == DEVICE_MD) {
      for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {
        mcl_seq.md_tracks[n].rotate_left();
      }
    }
#ifdef EXT_TRACKS
    else {
      for (uint8_t n = 0; n < NUM_EXT_TRACKS; n++) {
        mcl_seq.ext_tracks[n].rotate_left();
      }
    }
#endif
    break;
  case 4:
    if (opt_midi_device_capture == DEVICE_MD) {
      for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {
        mcl_seq.md_tracks[n].rotate_right();
      }
    }
#ifdef EXT_TRACKS
    else {
      for (uint8_t n = 0; n < NUM_EXT_TRACKS; n++) {
        mcl_seq.ext_tracks[n].rotate_right();
      }
    }
#endif
    break;
  }
}

void opt_reverse_track_handler() {

  if (opt_reverse == 2) {
    if (opt_midi_device_capture == DEVICE_MD) {
#ifdef OLED_DISPLAY
      //oled_display.textbox("REVERSE ", "MD TRACKS");
#endif
      for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {
        mcl_seq.md_tracks[n].reverse();
      }
    }
#ifdef EXT_TRACKS
    else {
#ifdef OLED_DISPLAY
      //oled_display.textbox("REVERSE ", "EXT TRACKS");
#endif
      for (uint8_t n = 0; n < NUM_EXT_TRACKS; n++) {
        mcl_seq.ext_tracks[n].reverse();
      }
    }
#endif
  }

  if (opt_reverse == 1) {
    if (opt_midi_device_capture == DEVICE_MD) {
#ifdef OLED_DISPLAY
      //oled_display.textbox("REVERSE ", "TRACK");
#endif
      mcl_seq.md_tracks[last_md_track].reverse();
    }
#ifdef EXT_TRACKS
    else {
#ifdef OLED_DISPLAY
      //oled_display.textbox("REVERSE ", "EXT TRACK");
#endif
      mcl_seq.ext_tracks[last_ext_track].reverse();
    }
#endif
  }
}

void seq_menu_handler() {
#ifndef OLED_DISPLAY
  SeqPage::show_seq_menu = false;
#endif
}
void step_menu_handler() {
#ifndef OLED_DISPLAY
  SeqPage::show_step_menu = false;
#endif
}

void SeqPage::config_as_trackedit() {

  seq_menu_page.menu.enable_entry(4, true);
  seq_menu_page.menu.enable_entry(5, false);
}

void SeqPage::config_as_lockedit() {

  seq_menu_page.menu.enable_entry(4, false);
  seq_menu_page.menu.enable_entry(5, true);
}

void SeqPage::loop() {
  if (last_md_track != MD.currentTrack) {
  select_track(midi_device, MD.currentTrack);
  }
  if (show_seq_menu) {
    seq_menu_page.loop();
    if (opt_midi_device_capture != DEVICE_MD && opt_trackid > 4) {
      // lock trackid to [1..4]
      opt_trackid = min(opt_trackid, 4);
      seq_menu_value_encoder.cur = opt_trackid;
    }
    return;
  } else if (show_step_menu) {
    step_menu_page.loop();
  }
}

void SeqPage::draw_page_index(bool show_page_index, uint8_t _playing_idx) {
#ifdef OLED_DISPLAY
  //  draw page index
  uint8_t pidx_x = pidx_x0;
  bool blink = MidiClock.getBlinkHint(true);

  uint8_t playing_idx;
  if (_playing_idx == 255) {
    if (midi_device == DEVICE_MD) {
      playing_idx =
          (mcl_seq.md_tracks[last_md_track].step_count -
           ((mcl_seq.md_tracks[last_md_track].step_count) / 16) * 16) /
          4;
    }
#ifdef EXT_TRACKS
    else {
      playing_idx =
          (mcl_seq.ext_tracks[last_ext_track].step_count -
           ((mcl_seq.ext_tracks[last_ext_track].step_count) / 16) * 16) /
          4;
    }
#endif
  } else {
    playing_idx = _playing_idx;
  }
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
#endif
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

  bool is_md = (midi_device == DEVICE_MD);
  if (!toggle_device) {
    is_md = true;
  }
#ifdef EXT_TRACKS
  bool ext_is_a4 = Analog4.connected;
#else
  bool ext_is_a4 = false;
#endif

  uint8_t track_id = last_md_track;
#ifdef EXT_TRACKS
  if (!is_md) {
    track_id = last_ext_track;
  }
#endif
  track_id += 1;

  //  draw current active track
  mcl_gui.draw_panel_number(track_id);

  //  draw MD/EXT label
  const char *str_ext = "MI";
  if (toggle_device) {
    if (ext_is_a4) {
      str_ext = "A4";
    }
  } else {
    str_ext = "  ";
  }
  mcl_gui.draw_panel_toggle("MD", str_ext, is_md);

  //  draw stop/play/rec state
  mcl_gui.draw_panel_status(recording, MidiClock.state == 2);

  if (display_page_index) {
    draw_page_index();
  }
  //  draw info lines
  mcl_gui.draw_panel_labels(info1, info2);

  if (show_seq_menu || show_step_menu) {
    constexpr uint8_t width = 52;
    oled_display.setFont(&TomThumb);
    oled_display.fillRect(128 - width - 2, 0, width + 2, 32, BLACK);
    if (show_step_menu) {
      step_menu_page.draw_menu(128 - width, 8, width);
    }
    if (show_seq_menu) {
      seq_menu_page.draw_menu(128 - width, 8, width);
    }
  }
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
  mcl_gui.draw_knob(i, title, text);
}

void SeqPage::draw_knob(uint8_t i, Encoder *enc, const char *title) {
  mcl_gui.draw_knob(i, enc, title);
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
