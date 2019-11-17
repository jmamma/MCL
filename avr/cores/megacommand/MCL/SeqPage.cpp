#include "MCL.h"
#include "SeqPage.h"

uint8_t SeqPage::page_select = 0;

uint8_t SeqPage::midi_device = DEVICE_MD;

uint8_t SeqPage::page_count = 4;
bool SeqPage::show_seq_menu = false;

uint8_t opt_resolution = 1;
uint8_t opt_trackid = 1;
uint8_t opt_copy = 0;
uint8_t opt_paste = 0;
uint8_t opt_clear = 0;
uint8_t opt_shift = 0;

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
      if (BUTTON_DOWN(Buttons.BUTTON3)) {
        opt_trackid = track + 1;
        select_track(device, track);
        redisplay = true;
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
      if (event->mask == EVENT_BUTTON_RELEASED) {
        note_interface.notes[track] = 0;
        GUI.ignoreNextEvent(event->source);
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
    route_page.update_globals();
    md_exploit.off();
    md_exploit.on();
    GUI.setPage(&page_select_page);
  }

#ifdef OLED_DISPLAY
  // activate show_seq_menu only if S2 press is not a key combination
  if (EVENT_PRESSED(event, Buttons.BUTTON3) && !show_seq_menu &&
      !BUTTON_DOWN(Buttons.BUTTON4)) {
    show_seq_menu = true;
    // capture current midi_device value
    opt_midi_device_capture = midi_device;
    // capture current page.
    opt_seqpage_capture = this;

    if (opt_midi_device_capture == DEVICE_MD) {
      DEBUG_PRINTLN("okay using MD for length update");
      opt_trackid = last_md_track + 1;
      opt_resolution = (mcl_seq.md_tracks[last_md_track].resolution);
    } else {
      opt_trackid = last_ext_track + 1;
      opt_resolution = (mcl_seq.ext_tracks[last_ext_track].resolution);
    }

    opt_param1_capture = (MCLEncoder *)encoders[0];
    opt_param2_capture = (MCLEncoder *)encoders[1];
    encoders[0] = &seq_menu_value_encoder;
    encoders[1] = &seq_menu_entry_encoder;
    seq_menu_page.init();
    return true;
  }
  if (EVENT_RELEASED(event, Buttons.BUTTON3)) {
    encoders[0] = opt_param1_capture;
    encoders[1] = opt_param2_capture;
    oled_display.clearDisplay();
    show_seq_menu = false;
    void (*row_func)() =
        seq_menu_page.menu.get_row_function(seq_menu_page.encoders[1]->cur);
    if (row_func != NULL) {
      (*row_func)();
      return true;
    }
    seq_menu_page.enter();
    return true;
  }
#else
  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
    // capture current midi_device value
    opt_midi_device_capture = midi_device;
    // capture current page.
    opt_seqpage_capture = this;
    GUI.pushPage(&seq_menu_page);
    return true;
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
                                bool show_current_step) {
  mcl_gui.draw_trigs(MCLGUI::seq_x0, MCLGUI::trig_y, offset, pattern_mask,
                     step_count, length);
}

void SeqPage::draw_pattern_mask(uint8_t offset, uint8_t device,
                                bool show_current_step) {
  if (device == DEVICE_MD) {
    auto &active_track = mcl_seq.md_tracks[last_md_track];
    draw_pattern_mask(offset, active_track.pattern_mask,
                      active_track.step_count, active_track.length,
                      show_current_step);
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

void opt_resolution_handler() {

  if (opt_midi_device_capture == DEVICE_MD) {
    DEBUG_PRINTLN("okay using MD for length update");
    (mcl_seq.md_tracks[last_md_track].resolution) = opt_resolution;
  }
#ifdef EXT_TRACKS
  else {
    (mcl_seq.ext_tracks[last_ext_track].resolution) = opt_resolution;
    seq_extstep_page.config_encoders();
  }
#endif
  opt_seqpage_capture->init();
}

void opt_clear_track_handler() {
  if (opt_midi_device_capture == DEVICE_MD) {
    if (opt_clear == 2) {
      for (uint8_t n = 0; n < 16; ++n) {
        mcl_seq.md_tracks[n].clear_track();
      }
    } else if (opt_clear == 1) {
      mcl_seq.md_tracks[last_md_track].clear_track();
    }
  } else {
    if (opt_clear == 2) {
      for (uint8_t n = 0; n < mcl_seq.num_ext_tracks; n++) {
        mcl_seq.ext_tracks[n].clear_track();
      }
    } else if (opt_clear == 1) {
      mcl_seq.ext_tracks[last_ext_track].clear_track();
    }
  }
  opt_clear = 0;
}

void opt_clear_locks_handler() {
  if (opt_midi_device_capture == DEVICE_MD) {
    if (opt_clear == 2) {
      for (uint8_t n = 0; n < 16; ++n) {
        mcl_seq.md_tracks[n].clear_locks();
      }
    } else if (opt_clear == 1) {
      mcl_seq.md_tracks[last_md_track].clear_locks();
    }
  } else {
    // TODO ext locks
  }
  opt_clear = 0;
}

void opt_clear_all_tracks_handler() {
  if (opt_midi_device_capture == DEVICE_MD) {
  } else {
    mcl_seq.ext_tracks[last_ext_track].clear_track();
  }
}

void opt_clear_all_locks_handler() {
  if (opt_midi_device_capture == DEVICE_MD) {
  } else {
    // TODO ext locks
  }
}

void opt_copy_track_handler() {
  if (opt_copy == 2) {
     if (opt_midi_device_capture == DEVICE_MD) { 
    mcl_clipboard.copy_sequencer();
     }
     else {
    mcl_clipboard.copy_sequencer(NUM_MD_TRACKS);
    }
  }
  if (opt_copy == 1) {
    if (opt_midi_device_capture == DEVICE_MD) {
    mcl_clipboard.copy_track = last_md_track;
    mcl_clipboard.copy_sequencer_track(last_md_track);
    }
    else {
    mcl_clipboard.copy_track = last_ext_track + NUM_MD_TRACKS;
    mcl_clipboard.copy_sequencer_track(last_ext_track + NUM_MD_TRACKS);
    }
  }
  opt_copy = 0;
}

void opt_paste_track_handler() {
  if (opt_paste == 2) {
    if (opt_midi_device_capture == DEVICE_MD) {
    mcl_clipboard.paste_sequencer();
    }
    else {
    mcl_clipboard.paste_sequencer(NUM_MD_TRACKS);
    }

  }
  if (opt_paste == 1) {
    if (opt_midi_device_capture == DEVICE_MD) {
    mcl_clipboard.paste_sequencer_track(mcl_clipboard.copy_track,
                                        last_md_track);
    }
    else {
    mcl_clipboard.paste_sequencer_track(mcl_clipboard.copy_track,
                                        last_ext_track + NUM_MD_TRACKS);
   }
  }
  opt_paste = 0;
}

void opt_shift_track_handler() {
  switch (opt_shift) {
  case 0:
    if (opt_midi_device_capture == DEVICE_MD) {
      mcl_seq.md_tracks[last_md_track].rotate_left();
    } else {
      mcl_seq.ext_tracks[last_ext_track].rotate_left();
    }

    break;
  case 1:
    if (opt_midi_device_capture == DEVICE_MD) {
      mcl_seq.md_tracks[last_md_track].rotate_right();
    } else {
      mcl_seq.ext_tracks[last_ext_track].rotate_right();
    }
    break;
  case 2:
    if (opt_midi_device_capture == DEVICE_MD) {
      for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {
        mcl_seq.md_tracks[n].rotate_left();
      }
    } else {
      for (uint8_t n = 0; n < NUM_EXT_TRACKS; n++) {
        mcl_seq.ext_tracks[n].rotate_left();
      }
    }
    break;
  case 3:
    if (opt_midi_device_capture == DEVICE_MD) {
      for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {
        mcl_seq.md_tracks[n].rotate_right();
      }
    } else {
      for (uint8_t n = 0; n < NUM_EXT_TRACKS; n++) {
        mcl_seq.ext_tracks[n].rotate_right();
      }
    }

    break;
  }
}

void SeqPage::config_as_trackedit() {

  seq_menu_page.menu.enable_entry(2, true);
  seq_menu_page.menu.enable_entry(3, false);
}

void SeqPage::config_as_lockedit() {

  seq_menu_page.menu.enable_entry(2, false);
  seq_menu_page.menu.enable_entry(3, true);
}

void SeqPage::loop() {

  if (show_seq_menu) {
    seq_menu_page.loop();
    if (opt_midi_device_capture != DEVICE_MD && opt_trackid > 4) {
      // lock trackid to [1..4]
      opt_trackid = min(opt_trackid, 4);
      seq_menu_value_encoder.cur = opt_trackid;
    }
    return;
  }
}

void SeqPage::draw_page_index(bool show_page_index) {
  //  draw page index
  uint8_t pidx_x = pidx_x0;
  bool blink = MidiClock.getBlinkHint(true);

  uint8_t playing_idx;
  if (midi_device == DEVICE_MD) {
   playing_idx = (mcl_seq.md_tracks[last_md_track].step_count) / 16;
  }
  else {
   playing_idx = (mcl_seq.ext_tracks[last_ext_track].step_count) / 16;
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
  const char *str_ext = "MI";
  if (ext_is_a4) {
    str_ext = "A4";
  }
  mcl_gui.draw_panel_toggle("MD", str_ext, is_md);

  //  draw stop/play/rec state
  mcl_gui.draw_panel_status(recording, MidiClock.state == 2);

  if (display_page_index) {
    draw_page_index();
  }
  //  draw info lines
  mcl_gui.draw_panel_labels(info1, info2);

  if (show_seq_menu) {
    constexpr uint8_t width = 52;
    oled_display.setFont(&TomThumb);
    oled_display.fillRect(128 - width - 2, 0, width + 2, 32, BLACK);
    seq_menu_page.draw_menu(128 - width, 8, width);
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
