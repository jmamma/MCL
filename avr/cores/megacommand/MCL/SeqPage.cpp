#include "MCL_impl.h"
#include "ResourceManager.h"

uint8_t SeqPage::page_select = 0;

MidiDevice *SeqPage::midi_device = &MD;

uint8_t SeqPage::last_param_id = 0;
uint8_t SeqPage::last_rec_event = 0;

uint8_t SeqPage::page_count = 4;

uint8_t SeqPage::pianoroll_mode = 0;

uint8_t SeqPage::mask_type = MASK_PATTERN;
uint8_t SeqPage::param_select = 0;

uint8_t SeqPage::last_pianoroll_mode = 0;

uint8_t SeqPage::velocity = 100;
uint8_t SeqPage::cond = 0;
uint8_t SeqPage::slide = true;
uint8_t SeqPage::md_micro = false;

bool SeqPage::show_seq_menu = false;
bool SeqPage::show_step_menu = false;
bool SeqPage::toggle_device = true;

uint8_t SeqPage::step_select = 255;

uint8_t opt_speed = 1;
uint8_t opt_trackid = 1;
uint8_t opt_copy = 0;
uint8_t opt_paste = 0;
uint8_t opt_clear = 0;
uint8_t opt_shift = 0;
uint8_t opt_reverse = 0;
uint8_t opt_clear_step = 0;
uint8_t opt_length = 0;
uint8_t opt_channel = 0;
uint8_t opt_undo = 255;
uint8_t opt_undo_track = 255;

uint16_t trigled_mask = 0;
uint16_t locks_on_step_mask = 0;

bool SeqPage::recording = false;

uint16_t SeqPage::deferred_timer = 0;
uint8_t SeqPage::last_midi_state = 0;

static MidiDevice *opt_midi_device_capture = &MD;
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
  recording = false;
  page_count = 4;
  ((MCLEncoder *)encoders[2])->handler = pattern_len_handler;
  seqpage_midi_events.setup_callbacks();
#ifdef OLED_DISPLAY
  classic_display = false;
  oled_display.clearDisplay();
#endif
  toggle_device = true;
  seq_menu_page.menu.enable_entry(SEQ_MENU_LENGTH, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_CHANNEL, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_MASK, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_ARP, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_TRANSPOSE, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_VEL, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_PROB, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_PIANOROLL, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_PARAMSELECT, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_SLIDE, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_POLY, false);

  /*
  if (mcl_cfg.track_select == 1) {
    seq_menu_page.menu.enable_entry(SEQ_MENU_TRACK, false);
  } else {
    seq_menu_page.menu.enable_entry(SEQ_MENU_TRACK, true);
  }
  */
  last_rec_event = 255;

  R.Clear();
  R.use_machine_names_short();
  R.use_machine_param_names();
}

void SeqPage::cleanup() {
  seqpage_midi_events.remove_callbacks();
  note_interface.init_notes();
  recording = false;
  clearLed2();
}

void SeqPage::bootstrap_record() {
  if (GUI.currentPage() != &seq_step_page &&
      GUI.currentPage() != &seq_param_page &&
      GUI.currentPage() != &seq_ptc_page) {
    GUI.setPage(&seq_step_page);
  }
  seq_step_page.recording = true;
  setLed2();
  MD.set_rec_mode(2);
  oled_display.textbox("REC", "");
}

void SeqPage::config_mask_info(bool silent) {
  switch (mask_type) {
  case MASK_PATTERN:
    strcpy(info2, "TRIG");
    break;
  case MASK_LOCK:
    strcpy(info2, "LOCK");
    break;
  case MASK_SLIDE:
    strcpy(info2, "SLIDE");
    break;
  case MASK_MUTE:
    strcpy(info2, "MUTE");
    break;
  }
  if (!silent) {
    char str[16] = "EDIT ";
    strcat(str, info2);
    MD.popup_text(str);
  }
}

void SeqPage::select_track(MidiDevice *device, uint8_t track) {
  if (device == &MD) {
    DEBUG_PRINTLN("setting md track");
    opt_undo = 255;
    last_md_track = track;
    auto &active_track = mcl_seq.md_tracks[last_md_track];
    MD.sync_seqtrack(active_track.length, active_track.speed,
                     active_track.step_count);
    if (mcl_cfg.track_select) {
      MD.currentTrack = track;
      MD.setStatus(0x22, track);
    }
  }
#ifdef EXT_TRACKS
  else {
    DEBUG_PRINTLN("setting ext track");
    last_ext_track = min(track, NUM_EXT_TRACKS - 1);
  }
#endif
  GUI.currentPage()->redisplay = true;
  GUI.currentPage()->config();
}

bool SeqPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    uint8_t port = event->port;
    MidiDevice *device = midi_active_peering.get_device(port);

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
      /* not required. pattern_len_handler will detect change when
       * encoder is updated below */
      /*
      if (SeqPage::midi_device == DEVICE_MD) {
        char str[4];
        itoa(step, str, 10);

        if (BUTTON_DOWN(Buttons.BUTTON3)) {
          oled_display.textbox("MD TRACKS LEN:", str);
          GUI.ignoreNextEvent(Buttons.BUTTON3);
          for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {
            mcl_seq.md_tracks[n].length = step;
          }
        }
        else {
        oled_display.textbox("MD TRACK LEN:", str);
        mcl_seq.md_tracks[last_md_track].length = step;
        }
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
*/
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

  if (EVENT_CMD(event)) {
    uint8_t key = event->source - 64;
    uint8_t step = note_interface.get_first_md_note() + (page_select * 16);
    if (note_interface.get_first_md_note() == 255) {
      step = 255;
    }
    if (event->mask == EVENT_BUTTON_PRESSED) {
      switch (key) {
      case MDX_KEY_LEFT:
        if (step != 255) {
          return false;
        }
        mcl_seq.md_tracks[last_md_track].rotate_left();
        return true;
      case MDX_KEY_RIGHT:
        if (step != 255) {
          return false;
        }
        mcl_seq.md_tracks[last_md_track].rotate_right();
        return true;
      case MDX_KEY_UP:
        if (step != 255) {
          return false;
        }
        if (trig_interface.is_key_down(MDX_KEY_FUNC)) {
          mcl_seq.md_tracks[last_md_track].reverse();
          return true;
        }
        return false;
      }
    }
    if (event->mask == EVENT_BUTTON_RELEASED) {
      switch (key) {
      case MDX_KEY_SCALE:
        goto scale_press;
      }
    }
  }

  // A not-ignored WRITE (BUTTON4) release event triggers sequence page select
  if (EVENT_RELEASED(event, Buttons.BUTTON4)) {
  scale_press:
    page_select += 1;
    if (page_select >= page_count ||
        page_select * 16 >= mcl_seq.md_tracks[last_md_track].length) {
      page_select = 0;
    }
    ElektronDevice *elektron_dev = midi_device->asElektronDevice();
    if (elektron_dev != nullptr) {
      elektron_dev->set_seq_page(page_select);
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

      if (opt_midi_device_capture == &MD) {
        auto &active_track = mcl_seq.md_tracks[last_md_track];
        opt_trackid = last_md_track + 1;
        opt_speed = active_track.speed;
        opt_length = active_track.length;
      } else {
#ifdef EXT_TRACKS
        opt_trackid = last_ext_track + 1;
        opt_speed = mcl_seq.ext_tracks[last_ext_track].speed;
        opt_length = mcl_seq.ext_tracks[last_ext_track].length;
        opt_channel = mcl_seq.ext_tracks[last_ext_track].channel + 1;
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
      row_func =
          seq_menu_page.menu.get_row_function(seq_menu_page.encoders[1]->cur);
    } else if (show_step_menu) {
      row_func =
          step_menu_page.menu.get_row_function(step_menu_page.encoders[1]->cur);
    }
    if (row_func != NULL) {
      row_func();
      show_seq_menu = false;
      show_step_menu = false;
      init();
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
    init();
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
        DEBUG_PRINTLN(F("okay using MD for length update"));
        opt_trackid = last_md_track + 1;

        opt_speed = get_md_speed(mcl_seq.md_tracks[last_md_track].speed);
      } else {
#ifdef EXT_TRACKS
        opt_trackid = last_ext_track + 1;
        opt_speed = get_ext_speed(mcl_seq.ext_tracks[last_ext_track].speed);
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

void SeqPage::draw_mask(uint8_t offset, uint8_t device,
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
             mcl_seq.ext_tracks[last_ext_track].speed) -
            (mcl_actions.start_clock32th /
             mcl_seq.ext_tracks[last_ext_track].speed)) -
           (mcl_seq.ext_tracks[last_ext_track].length *
            ((MidiClock.div32th_counter /
                  mcl_seq.ext_tracks[last_ext_track].speed -
              (mcl_actions.start_clock32th /
               mcl_seq.ext_tracks[last_ext_track].speed)) /
             (mcl_seq.ext_tracks[last_ext_track].length)));
       */
      uint8_t step_count = mcl_seq.ext_tracks[last_ext_track].step_count;
      uint8_t noteson = 0;
      uint8_t notesoff = 0;

      for (uint8_t a = 0; a < NUM_EXT_NOTES; a++) {

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

void SeqPage::draw_lock_mask(const uint8_t offset, const uint64_t &lock_mask,
                             const uint8_t step_count, const uint8_t length,
                             const bool show_current_step) {
  mcl_gui.draw_leds(MCLGUI::seq_x0, MCLGUI::led_y, offset, lock_mask,
                    step_count, length, show_current_step);
}

void SeqPage::draw_lock_mask(uint8_t offset, bool show_current_step) {
  auto &active_track = mcl_seq.md_tracks[last_md_track];
  uint64_t mask;
  active_track.get_mask(&mask, MASK_LOCKS_ON_STEP);
  draw_lock_mask(offset, mask, active_track.step_count, active_track.length,
                 show_current_step);
}

void SeqPage::draw_mask(const uint8_t offset, const uint64_t &pattern_mask,
                        const uint8_t step_count, const uint8_t length,
                        const uint64_t &mute_mask, const uint64_t &slide_mask,
                        const bool show_current_step) {
  mcl_gui.draw_trigs(MCLGUI::seq_x0, MCLGUI::trig_y, offset, pattern_mask,
                     step_count, length, mute_mask, slide_mask);
}

void SeqPage::draw_mask(uint8_t offset, uint8_t device,
                        bool show_current_step) {

  if (device == DEVICE_MD) {
    auto &active_track = mcl_seq.md_tracks[last_md_track];
    uint64_t mask, lock_mask, oneshot_mask = 0, slide_mask = 0;
    active_track.get_mask(&mask, MASK_PATTERN);
    uint64_t led_mask = 0;

    switch (mask_type) {
    case MASK_PATTERN:
      led_mask = mask;
      break;
    case MASK_LOCK:
      active_track.get_mask(&lock_mask, MASK_LOCK);
      led_mask = lock_mask;
      break;
    case MASK_MUTE:
      oneshot_mask = active_track.oneshot_mask;
      led_mask = oneshot_mask;
      break;
    case MASK_SLIDE:
      active_track.get_mask(&slide_mask, MASK_SLIDE);
      led_mask = slide_mask;
      break;
    }
    led_mask <<= (64 - active_track.length);
    led_mask >>= (64 - active_track.length);
    led_mask = led_mask >> offset;
    draw_mask(offset, mask, active_track.step_count, active_track.length,
              oneshot_mask, slide_mask, show_current_step);

    if (recording)
      return;
    mask <<= (64 - active_track.length);
    mask >>= (64 - active_track.length);

    uint64_t locks_on_step_mask_ = 0;
    active_track.get_mask(&locks_on_step_mask_, MASK_LOCKS_ON_STEP);
    locks_on_step_mask_ <<= (64 - active_track.length);
    locks_on_step_mask_ >>= (64 - active_track.length);

    if (led_mask != trigled_mask) {
      trigled_mask = led_mask;
      MD.set_trigleds(trigled_mask, TRIGLED_STEPEDIT);
      if (mask_type == MASK_MUTE) {
        MD.set_trigleds(mask, TRIGLED_STEPEDIT, 1);
      }
    }
    if (locks_on_step_mask_ != locks_on_step_mask) {
      locks_on_step_mask = locks_on_step_mask_;
      MD.set_trigleds(locks_on_step_mask, TRIGLED_STEPEDIT, 1);
    }
  }
}

// from knob value to step value
uint8_t SeqPage::translate_to_step_conditional(uint8_t condition,
                                               /*OUT*/ bool *plock) {
  if (condition > NUM_TRIG_CONDITIONS) {
    condition = condition - NUM_TRIG_CONDITIONS;
    *plock = true;
  } else {
    *plock = false;
  }
  return condition;
}

// from step value to knob value
uint8_t SeqPage::translate_to_knob_conditional(uint8_t condition,
                                               /*IN*/ bool plock) {
  if (plock) {
    condition = condition + NUM_TRIG_CONDITIONS;
  }
  return condition;
}

void SeqPage::draw_knob_conditional(uint8_t cond) {
  char K[4];
  conditional_str(K, cond);
  draw_knob(0, "COND", K);
}

void SeqPage::conditional_str(char *str, uint8_t cond, bool is_md) {
  if (cond == 0) {
    strcpy(str, "L1");
  } else {
    if (cond > NUM_TRIG_CONDITIONS) {
      cond = cond - NUM_TRIG_CONDITIONS;
    }

    if (cond <= 8) {
      strcpy(str, "L  ");
      str[1] = cond + '0';
    } else if (cond <= 13) {
      strcpy(str, "P  ");
      uint8_t prob[5] = {1, 2, 5, 7, 9};
      str[1] = prob[cond - 9] + '0';
    } else if (cond == 14) {
      strcpy(str, "1S ");
    }
    str[3] = '\0';
    if (seq_param1.getValue() > NUM_TRIG_CONDITIONS) {
      str[2] = '^';
      if (is_md) {
        str[2] = '+';
      }
    }
  }
}

void SeqPage::draw_knob_timing(uint8_t timing, uint8_t timing_mid) {
  char K[4];
  strcpy(K, "--");
  K[3] = '\0';

  if (timing == 0) {
  } else if ((timing < timing_mid) && (timing != 0)) {
    itoa(timing_mid - timing, K + 1, 10);
  } else {
    K[0] = '+';
    itoa(timing - timing_mid, K + 1, 10);
  }
  draw_knob(1, "UTIM", K);
}
#endif // OLED_DISPLAY

void pattern_len_handler(EncoderParent *enc) {
  MCLEncoder *enc_ = (MCLEncoder *)enc;
  if (!enc_->hasChanged()) {
    return;
  }
  if (SeqPage::midi_device == &MD) {
    DEBUG_PRINTLN(F("under 16"));
    if (BUTTON_DOWN(Buttons.BUTTON4)) {
      char str[4];
      itoa(enc_->cur, str, 10);
#ifdef OLED_DISPLAY
      oled_display.textbox("MD TRACKS LEN: ", str);
#endif
      GUI.ignoreNextEvent(Buttons.BUTTON4);
      for (uint8_t c = 0; c < 16; c++) {
        mcl_seq.md_tracks[c].set_length(enc_->cur);
      }
    } else {
      auto &active_track = mcl_seq.md_tracks[last_md_track];
      active_track.set_length(enc_->cur);
      MD.sync_seqtrack(active_track.length, active_track.speed,
                       active_track.step_count);
    }
  }
#ifdef EXT_TRACKS
  else {
    if (BUTTON_DOWN(Buttons.BUTTON4)) {
      for (uint8_t c = 0; c < mcl_seq.num_ext_tracks; c++) {
        char str[4];
        itoa(enc_->cur, str, 10);
        GUI.ignoreNextEvent(Buttons.BUTTON4);
#ifdef OLED_DISPLAY
        oled_display.textbox("EXT TRACKS LEN: ", str);
#endif
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

void opt_length_handler() {
  if (opt_midi_device_capture == &MD) {
    auto &active_track = mcl_seq.md_tracks[last_md_track];
    active_track.set_length(opt_length);
    MD.sync_seqtrack(active_track.length, active_track.speed,
                     active_track.step_count);
  } else {
    mcl_seq.ext_tracks[last_ext_track].buffer_notesoff();
    mcl_seq.ext_tracks[last_ext_track].set_length(opt_length);
  }
}

void opt_channel_handler() {
  if (opt_midi_device_capture == &MD) {
  } else {
    mcl_seq.ext_tracks[last_ext_track].buffer_notesoff();
    mcl_seq.ext_tracks[last_ext_track].channel = opt_channel - 1;
  }
}

void opt_mask_handler() { seq_step_page.config_mask_info(false); }

void opt_trackid_handler() {
  opt_seqpage_capture->select_track(opt_midi_device_capture, opt_trackid - 1);
}

void opt_speed_handler() {

  if (opt_midi_device_capture == &MD) {
    DEBUG_PRINTLN(F("okay using MD for length update"));
    if (BUTTON_DOWN(Buttons.BUTTON4)) {
      for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {
        mcl_seq.md_tracks[n].set_speed(opt_speed);
      }
      GUI.ignoreNextEvent(Buttons.BUTTON4);
    } else {
      auto &active_track = mcl_seq.md_tracks[last_md_track];
      active_track.set_speed(opt_speed);
      MD.sync_seqtrack(active_track.length, active_track.speed,
                       active_track.step_count);
    }
    seq_step_page.config_encoders();
  }
#ifdef EXT_TRACKS
  else {
    if (BUTTON_DOWN(Buttons.BUTTON4)) {
      for (uint8_t n = 0; n < NUM_EXT_TRACKS; n++) {
        mcl_seq.ext_tracks[n].set_speed(opt_speed);
      }
      GUI.ignoreNextEvent(Buttons.BUTTON4);
    } else {
      mcl_seq.ext_tracks[last_ext_track].set_speed(opt_speed);
      seq_extstep_page.config_encoders();
    }
  }
#endif
  opt_seqpage_capture->init();
}

void opt_clear_track_handler() {
  uint8_t copy = false;
  if (opt_undo != 255) {
    if (opt_undo != opt_clear) {
      opt_undo = 255;
      goto COPY;
    }
    opt_paste = opt_clear;
    opt_paste_track_handler();
    return;
  } else {
  COPY:
    copy = true;
  }
  if (opt_midi_device_capture == &MD) {
    if (opt_clear == 2) {

      MD.popup_text(2);
#ifdef OLED_DISPLAY
      oled_display.textbox("CLEAR MD ", "TRACKS");
#endif

      if (copy) {
        opt_copy_track_handler(opt_clear);
      }
      for (uint8_t n = 0; n < 16; ++n) {
        mcl_seq.md_tracks[n].clear_track();
      }
    } else if (opt_clear == 1) {
#ifdef OLED_DISPLAY
      oled_display.textbox("CLEAR TRACK", "");
#endif
      MD.popup_text(5);
      if (copy) {
        opt_copy_track_handler(opt_clear);
      }
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

  if (opt_midi_device_capture == &MD) {
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
    auto &active_track = mcl_seq.ext_tracks[last_ext_track];
    if (opt_clear == 2) {
      oled_display.textbox("CLEAR ", "LOCKS");
      for (uint8_t n = 0; n < NUM_LOCKS; n++) {
        active_track.clear_track_locks(n);
      }
    }
    if (opt_clear == 1) {
      oled_display.textbox("CLEAR ", "LOCK");
      if (SeqPage::pianoroll_mode > 0) {
        active_track.clear_track_locks(SeqPage::pianoroll_mode - 1);
      }
    }
    // TODO ext locks
  }
  opt_clear = 0;
}

void opt_clear_all_tracks_handler() {
  if (opt_midi_device_capture == &MD) {
  }
#ifdef EXT_TRACKS
  else {
    mcl_seq.ext_tracks[last_ext_track].clear_track();
  }
#endif
}

void opt_clear_all_locks_handler() {
  if (opt_midi_device_capture == &MD) {
  }
#ifdef EXT_TRACKS
  else {
    // TODO ext locks
  }
#endif
}

void opt_copy_track_handler() { opt_copy_track_handler(255); }

void opt_copy_track_handler(uint8_t op) {
  bool silent = false;
  opt_undo = 255;
  if (op != 255) {
    opt_copy = op;
    opt_undo = op;
    silent = true;
  }
  DEBUG_PRINTLN("copying");
  if (opt_copy == 2) {

    if (opt_midi_device_capture == &MD) {
      if (!silent) {
#ifdef OLED_DISPLAY
        oled_display.textbox("COPY MD ", "TRACKS");
#endif
        MD.popup_text(1);
      }
      mcl_clipboard.copy_sequencer();
    }
#ifdef EXT_TRACKS
    else {
      if (!silent) {
#ifdef OLED_DISPLAY
        oled_display.textbox("COPY EXT ", "TRACKS");
#endif
      }
      mcl_clipboard.copy_sequencer(NUM_MD_TRACKS);
    }
#endif
  }
  if (opt_copy == 1) {
    if (opt_midi_device_capture == &MD) {
      if (!silent) {
#ifdef OLED_DISPLAY
        oled_display.textbox("COPY TRACK", "");
#endif
        MD.popup_text(4);
      }
      mcl_clipboard.copy_track = last_md_track;
      mcl_clipboard.copy_sequencer_track(last_md_track);
    }
#ifdef EXT_TRACKS
    else {
      if (!silent) {
#ifdef OLED_DISPLAY
        oled_display.textbox("COPY EXT ", "TRACK");
#endif
      }
      mcl_clipboard.copy_track = last_ext_track + NUM_MD_TRACKS;
      mcl_clipboard.copy_sequencer_track(last_ext_track + NUM_MD_TRACKS);
    }
#endif
  }
  opt_copy = 0;
}

void opt_paste_track_handler() {
  bool undo = false;
  if (opt_undo != 255) {
    undo = true;
  }
  if (opt_paste == 2) {

    if (opt_midi_device_capture == &MD) {
      if (!undo) {
        oled_display.textbox("PASTE MD ", "TRACKS");
        MD.popup_text(3);
      } else {
        oled_display.textbox("UNDO ", "TRACKS");
        MD.popup_text(22);
      }
      mcl_clipboard.paste_sequencer();
    }
#ifdef EXT_TRACKS
    else {
      oled_display.textbox("PASTE EXT ", "TRACKS");
      mcl_clipboard.paste_sequencer(NUM_MD_TRACKS);
    }
#endif
  }
  if (opt_paste == 1) {
    if (opt_midi_device_capture == &MD) {
      if (!undo) {
        oled_display.textbox("PASTE TRACK", "");
        MD.popup_text(6);
      } else {
        oled_display.textbox("UNDO TRACK", "");
        MD.popup_text(23);
      }
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
  opt_undo = 255;
  opt_paste = 0;
}

void opt_clear_page_handler() {
  if (opt_undo != 255) {
    if (opt_undo != PAGE_UNDO) {
      opt_undo = 255;
      goto CLEAR;
    }
    opt_paste_page_handler();
    return;
  } else {
  CLEAR:
    opt_copy_page_handler(PAGE_UNDO);
  }
#ifdef OLED_DISPLAY
  oled_display.textbox("CLEAR PAGE", "");
#endif
  MD.popup_text(57);
  MDSeqStep empty_step;
  memset(&empty_step, 0, sizeof(empty_step));
  for (uint8_t n = 0; n < 16; n++) {
    uint8_t step = n + SeqPage::page_select * 16;
    if (step >= mcl_seq.md_tracks[last_md_track].length) {
      return;
    }
    mcl_seq.md_tracks[last_md_track].paste_step(step, &empty_step);
  }
}

void opt_copy_page_handler() { opt_copy_page_handler(255); }

void opt_copy_page_handler(uint8_t op) {
  bool silent = false;
  opt_undo = 255;
  if (op != 255) {
    opt_undo = op;
    silent = true;
  }

  if (!silent) {
#ifdef OLED_DISPLAY
    oled_display.textbox("COPY PAGE", "");
#endif
    MD.popup_text(54);
  }
  for (uint8_t n = 0; n < 16; n++) {
    uint8_t step = n + SeqPage::page_select * 16;
    if (step >= mcl_seq.md_tracks[last_md_track].length) {
      return;
    }
    mcl_seq.md_tracks[last_md_track].copy_step(step, &mcl_clipboard.steps[n]);
  }
}

void opt_paste_page_handler() {
  if (opt_undo == PAGE_UNDO) {
    opt_undo = 255;
    oled_display.textbox("UNDO PAGE", "");
    MD.popup_text(55);
  } else {
#ifdef OLED_DISPLAY
    oled_display.textbox("PASTE PAGE", "");
#endif
    MD.popup_text(56);
  }
  for (uint8_t n = 0; n < 16; n++) {
    uint8_t step = n + SeqPage::page_select * 16;
    if (step >= mcl_seq.md_tracks[last_md_track].length) {
      return;
    }
    mcl_seq.md_tracks[last_md_track].paste_step(step, &mcl_clipboard.steps[n]);
  }
}

void opt_clear_step_handler() {
#ifdef OLED_DISPLAY
  oled_display.textbox("CLEAR STEP", "");
#endif
  MD.popup_text(14);
  MDSeqStep empty_step;
  memset(&empty_step, 0, sizeof(empty_step));
  mcl_seq.md_tracks[last_md_track].paste_step(
      SeqPage::step_select + SeqPage::page_select * 16, &empty_step);
}

void opt_copy_step_handler() {
#ifdef OLED_DISPLAY
  oled_display.textbox("COPY STEP", "");
#endif
  MD.popup_text(13);
  mcl_seq.md_tracks[last_md_track].copy_step(SeqPage::step_select +
                                                 SeqPage::page_select * 16,
                                             &mcl_clipboard.steps[0]);
}

void opt_paste_step_handler() {
#ifdef OLED_DISPLAY
  oled_display.textbox("PASTE STEP", "");
#endif
  MD.popup_text(15);
  mcl_seq.md_tracks[last_md_track].paste_step(SeqPage::step_select +
                                                  SeqPage::page_select * 16,
                                              &mcl_clipboard.steps[0]);
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
    MD.popup_text(14);
  }
  for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {
    if (note_interface.notes[n] == 1) {

      if (opt_midi_device_capture == &MD) {
        mcl_seq.md_tracks[last_md_track].clear_step_locks(
            n + SeqPage::page_select * 16);
      } else {
        //        mcl_seq.ext_tracks[last_ext_track].clear_step_locks(
        //          SeqPage::step_select + SeqPage::page_select * 16);
      }
    }
  }
  opt_clear_step = 0;
}

void opt_shift_track_handler() {
  switch (opt_shift) {
  case 1:
    if (opt_midi_device_capture == &MD) {
      mcl_seq.md_tracks[last_md_track].rotate_left();
    }
#ifdef EXT_TRACKS
    else {
      mcl_seq.ext_tracks[last_ext_track].rotate_left();
    }
#endif
    break;
  case 2:
    if (opt_midi_device_capture == &MD) {
      mcl_seq.md_tracks[last_md_track].rotate_right();
    }
#ifdef EXT_TRACKS
    else {
      mcl_seq.ext_tracks[last_ext_track].rotate_right();
    }
#endif
    break;
  case 3:
    if (opt_midi_device_capture == &MD) {
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
    if (opt_midi_device_capture == &MD) {
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
    if (opt_midi_device_capture == &MD) {
#ifdef OLED_DISPLAY
      // oled_display.textbox("REVERSE ", "MD TRACKS");
#endif
      for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {
        mcl_seq.md_tracks[n].reverse();
      }
    }
#ifdef EXT_TRACKS
    else {
#ifdef OLED_DISPLAY
      // oled_display.textbox("REVERSE ", "EXT TRACKS");
#endif
      for (uint8_t n = 0; n < NUM_EXT_TRACKS; n++) {
        mcl_seq.ext_tracks[n].reverse();
      }
    }
#endif
  }

  if (opt_reverse == 1) {
    if (opt_midi_device_capture == &MD) {
#ifdef OLED_DISPLAY
      // oled_display.textbox("REVERSE ", "TRACK");
#endif
      mcl_seq.md_tracks[last_md_track].reverse();
    }
#ifdef EXT_TRACKS
    else {
#ifdef OLED_DISPLAY
      // oled_display.textbox("REVERSE ", "EXT TRACK");
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

  seq_menu_page.menu.enable_entry(SEQ_MENU_CLEAR_TRACK, true);
  seq_menu_page.menu.enable_entry(SEQ_MENU_CLEAR_LOCKS, false);
}

void SeqPage::config_as_lockedit() {

  seq_menu_page.menu.enable_entry(SEQ_MENU_CLEAR_TRACK, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_CLEAR_LOCKS, true);
}

void SeqPage::loop() {
  if (deferred_timer != 0 &&
      clock_diff(deferred_timer, slowclock) > render_defer_time) {
    deferred_timer = 0;
    DEBUG_DUMP("redisplay");
    redisplay = true;
  }

  if (encoders[0]->hasChanged() || encoders[1]->hasChanged() ||
      encoders[2]->hasChanged() || encoders[3]->hasChanged()) {
    DEBUG_DUMP("queue redraw");
    queue_redraw();
  }

  if (last_midi_state != MidiClock.state) {
    last_midi_state = MidiClock.state;
    DEBUG_DUMP("hii")
    redisplay = true;
  }

  if (last_md_track != MD.currentTrack) {
    select_track(&MD, MD.currentTrack);
  }
  if (show_seq_menu) {
    seq_menu_page.loop();
    if (opt_midi_device_capture != &MD && opt_trackid > NUM_EXT_TRACKS) {
      // lock trackid to [1..4]
      opt_trackid = min(opt_trackid, NUM_EXT_TRACKS);
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
    if (midi_device == &MD) {
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

  bool is_md = (midi_device == &MD);
  const char *int_name = midi_active_peering.get_device(UART1_PORT)->name;
  const char *ext_name = midi_active_peering.get_device(UART2_PORT)->name;

  uint8_t track_id = last_md_track;
#ifdef EXT_TRACKS
  if (!is_md) {
    track_id = last_ext_track;
  }
#endif
  track_id += 1;

  //  draw current active track
  mcl_gui.draw_panel_number(track_id);

  mcl_gui.draw_panel_toggle(int_name, ext_name, is_md);

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

void SeqPageMidiEvents::onMidiStartCallback() {
  if (SeqPage::recording) {
    oled_display.textbox("REC", "");
  }
}

void SeqPageMidiEvents::setup_callbacks() {
  MidiClock.addOnMidiStartCallback(
      this, (midi_clock_callback_ptr_t)&SeqPageMidiEvents::onMidiStartCallback);
  //   Midi.addOnControlChangeCallback(
  //      this,
  //      (midi_callback_ptr_t)&SeqPageMidiEvents::onControlChangeCallback_Midi);
}

void SeqPageMidiEvents::remove_callbacks() {
  MidiClock.addOnMidiStartCallback(
      this, (midi_clock_callback_ptr_t)&SeqPageMidiEvents::onMidiStartCallback);
  //  Midi.removeOnControlChangeCallback(
  //      this,
  //    (midi_callback_ptr_t)&SeqPageMidiEvents::onControlChangeCallback_Midi);
}

void SeqPageMidiEvents::onControlChangeCallback_Midi(uint8_t *msg) {}
