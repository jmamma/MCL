#include "MCL_impl.h"
#include "ResourceManager.h"

uint8_t SeqPage::page_select = 0;

MidiDevice *SeqPage::midi_device = &MD;
MidiDevice *opt_midi_device_capture;

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
bool SeqPage::toggle_device = true;

uint16_t SeqPage::mute_mask = 0;

uint8_t SeqPage::step_select = 255;

bool SeqPage::is_midi_model = false;

uint32_t SeqPage::last_md_model = 255;

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

uint8_t SeqPage::last_midi_state = 0;
uint8_t SeqPage::last_step = 255;

static MCLEncoder *opt_param1_capture = nullptr;
static MCLEncoder *opt_param2_capture = nullptr;

MusicalNotes number_to_note;

uint8_t copy_mask = 0;

void SeqPage::setup() {}

void SeqPage::check_and_set_page_select() {
  reset_undo();
  if (page_select >= page_count ||
      page_select * 16 >= mcl_seq.md_tracks[last_md_track].length) {
    page_select = 0;
  }
  ElektronDevice *elektron_dev = midi_device->asElektronDevice();
  if (elektron_dev != &MD) {
  DEBUG_PRINTLN("bad device");
  }
  if (elektron_dev != nullptr) {
    elektron_dev->set_seq_page(page_select);
  }
}

void SeqPage::toggle_record() {
  recording = !recording;
  if (recording) {
    enable_record();
  } else {
    disable_record();
  }
}

void SeqPage::enable_record() {
  MD.set_rec_mode(2);
  recording = true;
  setLed2();
  oled_display.textbox("REC", "");
}

void SeqPage::disable_record() {
  MD.set_rec_mode((mcl.currentPage() == SEQ_STEP_PAGE));
  recording = false;
  clearLed2();
}

void SeqPage::init() {
  uint8_t _midi_lock_tmp = MidiUartParent::handle_midi_lock;
  MidiUartParent::handle_midi_lock = 0;
  disable_record();
  page_count = 4;
  ((MCLEncoder *)encoders[2])->handler = pattern_len_handler;
  config_encoders();
  seqpage_midi_events.setup_callbacks();

  toggle_device = true;
  DEBUG_PRINTLN("seq page init");

  seq_menu_page.menu.enable_entry(SEQ_MENU_DEVICE, false);
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
  seq_menu_page.menu.enable_entry(SEQ_MENU_SOUND, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_AUTOMATION, false);

  seq_menu_page.menu.enable_entry(SEQ_MENU_LENGTH_MD, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_LENGTH_EXT, false);
  /*
  if (mcl_cfg.track_select == 1) {
    seq_menu_page.menu.enable_entry(SEQ_MENU_TRACK, false);
  } else {
    seq_menu_page.menu.enable_entry(SEQ_MENU_TRACK, true);
  }
  */
  last_rec_event = 255;
  last_md_model = MD.kit.models[MD.currentTrack];

  R.Clear();
  R.use_icons_knob();
  R.use_machine_names_short();
  R.use_machine_param_names();
  MidiUartParent::handle_midi_lock = _midi_lock_tmp;

}

void SeqPage::cleanup() {
  seqpage_midi_events.remove_callbacks();
  note_interface.init_notes();
  disable_record();
  if (show_seq_menu) {
    encoders[0] = opt_param1_capture;
    encoders[1] = opt_param2_capture;
    show_seq_menu = false;
  }
}

void SeqPage::params_reset() {
  if (MidiClock.state != 2) {
    MDTrack md_track;
    md_track.machine.model = MD.kit.models[last_md_track];
    MD.assignMachineBulk(last_md_track, &md_track.machine, 255, 0, true);
    MD.setTrackParam(last_md_track, 0, MD.kit.params[last_md_track][0]);
  }
}

void SeqPage::bootstrap_record() {
  if (mcl.currentPage() != SEQ_STEP_PAGE &&
      mcl.currentPage() != SEQ_EXTSTEP_PAGE &&
      mcl.currentPage() != SEQ_PTC_PAGE) {
    mcl.setPage(SEQ_STEP_PAGE);
  }
  trig_interface.send_md_leds(TRIGLED_OVERLAY);
  enable_record();
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
    if (mask_type == MASK_PATTERN) {
      MD.popup_text(-1, 2);
    } else {
      MD.popup_text(str, 1);
    }
  }
}

void SeqPage::toggle_ext_mask(uint8_t track) {
  if (track > 6) {
    track -= 8;
    if (track >= mcl_seq.num_ext_tracks) {
      return;
    }
    mcl_seq.ext_tracks[track].toggle_mute();
  } else {
    if (track >= mcl_seq.num_ext_tracks) {
      return;
    }
    MidiDevice *dev = midi_active_peering.get_device(UART2_PORT);
    midi_device = dev;
    select_track(dev, track);
    opt_trackid = last_ext_track + 1;
    opt_speed = mcl_seq.ext_tracks[last_ext_track].speed;
    opt_length = mcl_seq.ext_tracks[last_ext_track].length;
    opt_channel = mcl_seq.ext_tracks[last_ext_track].channel + 1;
  }
}

void SeqPage::select_track(MidiDevice *device, uint8_t track, bool send) {
  reset_undo();
  if (device == &MD) {
    DEBUG_PRINTLN("setting md track");
    opt_undo = 255;
    DEBUG_PRINTLN(track);
    if (track >= NUM_MD_TRACKS) { return; }
    last_md_track = track;
    is_midi_model = ((MD.kit.models[last_md_track] & 0xF0) == MID_01_MODEL);
    auto &active_track = mcl_seq.md_tracks[last_md_track];
    MD.sync_seqtrack(active_track.length, active_track.speed,
                     active_track.step_count);
    check_and_set_page_select();
    if (mcl_cfg.track_select && send) {
      MD.currentTrack = track;
      MD.setStatus(0x22, track);
    }
  }
#ifdef EXT_TRACKS
  else {
    DEBUG_PRINTLN("setting ext track");
    last_ext_track = min(track, NUM_EXT_TRACKS - 1);
    auto &active_track = mcl_seq.ext_tracks[last_ext_track];
    MD.sync_seqtrack(min(active_track.length,64), active_track.speed,
                     active_track.step_count);
  }
#endif
  GUI.currentPage()->config();
  //config_encoders();
}

bool SeqPage::display_mute_mask(MidiDevice* device, uint8_t offset) {
  uint16_t last_mute_mask = mute_mask;

  bool is_md_device = (device == &MD);
  mute_mask = 0;

  uint8_t len = is_md_device ? mcl_seq.num_md_tracks : mcl_seq.num_ext_tracks;

  //Hack to display last_ext_track
  if (offset > 0 && !is_md_device) {
    SET_BIT16(mute_mask, last_ext_track);
  }

  for (uint8_t n = 0; n < len; n++) {

   SeqTrack *seq_track = is_md_device ? (SeqTrack*) &mcl_seq.md_tracks[n] : (SeqTrack*) &mcl_seq.ext_tracks[n];

   if (seq_track->mute_state == SEQ_MUTE_OFF) {
      uint8_t d = offset + n;
      if (d < 16) {
        SET_BIT16(mute_mask, d);
      }
    }
  }
  if (last_mute_mask != mute_mask) {
    MD.set_trigleds(mute_mask, is_md_device ? TRIGLED_MUTE : TRIGLED_EXCLUSIVE);
    return true;
  }
  return false;
}


bool SeqPage::handleEvent(gui_event_t *event) {

  if (EVENT_CMD(event)) {
    if (trig_interface.is_key_down(MDX_KEY_PATSONG)) {
      return seq_menu_page.handleEvent(event);
    }
    uint8_t key = event->source - 64;
    uint8_t first_note = note_interface.get_first_md_note();
    if (first_note != 255) { return false; }

    if (event->mask == EVENT_BUTTON_PRESSED &&
        trig_interface.is_key_down(MDX_KEY_FUNC)) {
      switch (key) {
      case MDX_KEY_LEFT:
       mcl_seq.md_tracks[last_md_track].rotate_left();
        return true;
      case MDX_KEY_RIGHT:
        mcl_seq.md_tracks[last_md_track].rotate_right();
        return true;
      case MDX_KEY_UP:
        mcl_seq.md_tracks[last_md_track].reverse();
        return true;
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
    check_and_set_page_select();
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
    // If MD trig is held and BUTTON3 is pressed, launch note menu
    if (!show_seq_menu) {
      show_seq_menu = true;
      if (opt_midi_device_capture == &MD) {
        auto &active_track = mcl_seq.md_tracks[last_md_track];
        opt_trackid = last_md_track + 1;
        opt_speed = active_track.speed;
        opt_length = active_track.length;
      } else {
        opt_trackid = last_ext_track + 1;
        opt_speed = mcl_seq.ext_tracks[last_ext_track].speed;
        opt_length = mcl_seq.ext_tracks[last_ext_track].length;
        opt_channel = mcl_seq.ext_tracks[last_ext_track].channel + 1;
      }

      opt_param1_capture = (MCLEncoder *)encoders[0];
      opt_param2_capture = (MCLEncoder *)encoders[1];
      encoders[0] = &seq_menu_value_encoder;
      encoders[1] = &seq_menu_entry_encoder;
      seq_menu_page.init();
      seq_menu_page.gen_menu_device_names();
      mcl_cfg.seq_dev = opt_midi_device_capture == &MD ? UART1_PORT : UART2_PORT;
      return true;
    }
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON3)) {
    encoders[0] = opt_param1_capture;
    encoders[1] = opt_param2_capture;
    //oled_display.clearDisplay();
    void (*row_func)();
    if (show_seq_menu) {
      row_func =
          seq_menu_page.menu.get_row_function(seq_menu_page.encoders[1]->cur);
      MidiDevice* old_dev = midi_device;
      midi_device = midi_active_peering.get_device(mcl_cfg.seq_dev);
      if (old_dev == midi_device) {
        opt_speed_handler();
        opt_length_handler();
        opt_channel_handler();
      }
    }
    if (row_func != NULL) {
      row_func();
      show_seq_menu = false;
      init();
      return true;
    }
    if (show_seq_menu && seq_menu_page.enter()) {
      show_seq_menu = false;
      return true;
    }

    show_seq_menu = false;
    init_encoders_used_clock();
    init();
    return true;
  }

  return false;
}

void SeqPage::draw_lock_mask(const uint8_t offset, const uint64_t &lock_mask,
                             const uint8_t step_count, const uint8_t length,
                             const bool show_current_step) {
  mcl_gui.draw_leds(MCLGUI::seq_x0, MCLGUI::led_y, offset, lock_mask,
                    step_count, length, show_current_step);
}

void SeqPage::shed_mask(uint64_t &mask, uint8_t length, uint8_t offset) {
  mask <<= (64 - length);
  mask >>= (64 - length);
  mask = mask >> offset;
}

void SeqPage::draw_lock_mask(uint8_t offset, bool show_current_step) {
  auto &active_track = mcl_seq.md_tracks[last_md_track];
  uint64_t mask;
  active_track.get_mask(&mask, MASK_LOCKS_ON_STEP);
  shed_mask(mask, active_track.length, 0);
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
    uint64_t mask, lock_mask, mute_mask = 0, slide_mask = 0;
    active_track.get_mask(&mask, MASK_PATTERN);
    uint64_t led_mask = 0;

    switch (mask_type) {
    case MASK_PATTERN:
      led_mask = mask;
      mute_mask = active_track.mute_mask;
      break;
    case MASK_LOCK:
      active_track.get_mask(&lock_mask, MASK_LOCK);
      led_mask = lock_mask;
      mask = lock_mask;
      break;
    case MASK_MUTE:
      mute_mask = active_track.mute_mask;
      led_mask = mute_mask;
      break;
    case MASK_SLIDE:
      active_track.get_mask(&slide_mask, MASK_SLIDE);
      led_mask = slide_mask;
      break;
    }
    shed_mask(led_mask, active_track.length, offset);
    draw_mask(offset, mask, active_track.step_count, active_track.length,
              mute_mask, slide_mask, show_current_step);

    if (recording)
      return;

    uint64_t locks_on_step_mask_ = 0;

    active_track.get_mask(&locks_on_step_mask_, MASK_LOCKS_ON_STEP);
    shed_mask(locks_on_step_mask_, active_track.length, offset);

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
/*
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
*/
void SeqPage::conditional_str(char *str, uint8_t cond, bool is_md) {
    if (str == nullptr) return;

    if (cond == 0) {
        str[0] = 'L'; str[1] = '1'; str[2] = '\0';
    } else {
        if (cond > NUM_TRIG_CONDITIONS) {
            cond -= NUM_TRIG_CONDITIONS;
        }

        if (cond <= 8) {
            str[0] = 'L'; str[1] = cond + '0'; str[2] = '\0';
        } else if (cond <= 13) {
            static const uint8_t prob[5] = {1, 2, 5, 7, 9};
            str[0] = 'P'; str[1] = prob[cond - 9] + '0'; str[2] = '\0';
        } else if (cond == 14) {
            str[0] = '1'; str[1] = 'S'; str[2] = '\0';
        }

        if (seq_param1.getValue() > NUM_TRIG_CONDITIONS) {
            str[2] = is_md ? '+' : '^';
            str[3] = '\0';
        }
    }
}

void SeqPage::draw_knob_timing(uint8_t timing, uint8_t timing_mid) {
  char K[4];
  strcpy(K, "--");
  K[3] = '\0';

  if (timing == 0) {
  } else if ((timing < timing_mid) && (timing != 0)) {
    mcl_gui.put_value_at(timing_mid - timing, K + 1);
  } else {
    K[0] = '+';
    mcl_gui.put_value_at(timing - timing_mid, K + 1);
  }
  draw_knob(1, "UTIM", K);
}

void SeqPage::length_handler(uint8_t length, bool multi) {
  bool is_poly = IS_BIT_SET16(mcl_cfg.poly_mask, last_md_track);
  if (opt_midi_device_capture == &MD) {

    if (multi) {
      for (uint8_t c = 0; c < 16; c++) {
        mcl_seq.md_tracks[c].set_length(length);
      }
    } else {
      if ((mcl_cfg.poly_mask) && (is_poly)) {
        for (uint8_t c = 0; c < 16; c++) {
          if (IS_BIT_SET16(mcl_cfg.poly_mask, c)) {
            mcl_seq.md_tracks[c].set_length(length);
          }
        }
      } else {
        mcl_seq.md_tracks[last_md_track].set_length(length);
      }
    }
    auto &active_track = mcl_seq.md_tracks[last_md_track];
    MD.sync_seqtrack(active_track.length, active_track.speed,
                     active_track.step_count);
  } else {
    if (multi) {
      for (uint8_t c = 0; c < NUM_EXT_TRACKS; c++) {
        mcl_seq.ext_tracks[c].set_length(length);
         if (last_ext_track == c) { seq_extparam4.cur = length; }
      }
    } else {
      mcl_seq.ext_tracks[last_ext_track].set_length(length);
      seq_extparam4.cur = length;
    }
  }
}


void pattern_len_handler(EncoderParent *enc) {
  MCLEncoder *enc_ = (MCLEncoder *)enc;
  if (!enc_->hasChanged()) {
    return;
  }
  seq_step_page.length_handler(enc_->cur, BUTTON_DOWN(Buttons.BUTTON4));
  GUI.ignoreNextEvent(Buttons.BUTTON4);
}

void opt_length_handler() {
  seq_step_page.length_handler(opt_length);
}

void opt_channel_handler() {
  if (opt_midi_device_capture == &MD) {
  } else {
    uint8_t chan = opt_channel - 1;
    if (mcl_seq.ext_tracks[last_ext_track].channel != chan) {
      mcl_seq.ext_tracks[last_ext_track].buffer_notesoff();
      mcl_seq.ext_tracks[last_ext_track].channel = chan;
    }
  }
}

void opt_mask_handler() { seq_step_page.config_mask_info(false); }

void opt_trackid_handler() {
  seq_step_page.select_track(opt_midi_device_capture, opt_trackid - 1);
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
      uint8_t last_speed = active_track.speed;
      if (opt_speed != last_speed) {
        active_track.set_speed(opt_speed);
        MD.sync_seqtrack(active_track.length, active_track.speed,
                         active_track.step_count);
      }
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
      uint8_t last_speed = mcl_seq.ext_tracks[last_ext_track].speed;
      if (opt_speed != last_speed) {
        mcl_seq.ext_tracks[last_ext_track].set_speed(opt_speed);
        seq_extstep_page.config_encoders();
      }
    }
  }
#endif
//  opt_seqpage_capture->init();
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
  if (opt_midi_device_capture == &MD || !(mcl.currentPage() == SEQ_PTC_PAGE || mcl.currentPage() == SEQ_EXTSTEP_PAGE)) {
    if (opt_clear == 2) {

      MD.popup_text(2);
      oled_display.textbox("CLEAR MD ", "TRACKS");
      oled_display.display();
      uint8_t old_mutes[16];
      for (uint8_t n = 0; n < 16; n++) {
        old_mutes[n] = mcl_seq.md_tracks[n].mute_state;
        mcl_seq.md_tracks[n].mute_state = SEQ_MUTE_ON;
      }
      if (copy) {
        opt_copy_track_handler(opt_clear);
      }
      for (uint8_t n = 0; n < 16; ++n) {
        mcl_seq.md_tracks[n].clear_track();
        mcl_seq.md_tracks[n].mute_state = old_mutes[n];
      }
    } else if (opt_clear == 1) {
      bool is_poly = IS_BIT_SET16(mcl_cfg.poly_mask, last_md_track);
      const char *str = "CLEAR TRACK";
      if (is_poly) {
        str = "CLEAR POLY TRACKS";
      }
      oled_display.textbox(str, "");
      oled_display.display();
      MD.popup_text((char*) str);
      if (copy) {
        opt_copy_track_handler(opt_clear);
      }
      if (is_poly) {
        for (uint8_t c = 0; c < 16; c++) {
          if (IS_BIT_SET16(mcl_cfg.poly_mask, c)) {
            mcl_clipboard.copy_sequencer_track(c);
            mcl_seq.md_tracks[c].clear_track();
          }
        }
      } else {
        mcl_seq.md_tracks[last_md_track].clear_track();
      }
    }
  } else {
    if (copy) {
      opt_copy_track_handler(opt_clear);
    }
    const char *str = "CLEAR EXT TRACK";
    if (opt_clear == 2) {
      for (uint8_t n = 0; n < mcl_seq.num_ext_tracks; n++) {
        str = "CLEAR EXT TRACKS";
        mcl_seq.ext_tracks[n].clear_track();
      }
    } else if (opt_clear == 1) {
      mcl_seq.ext_tracks[last_ext_track].clear_track();
    }
    if (opt_clear) {
      oled_display.textbox((char*) str, "");
      MD.popup_text((char*) str);
    }
  }
  opt_clear = 0;
}

void opt_clear_locks_handler() {

  if (opt_midi_device_capture == &MD) {
    if (opt_clear == 2) {
      for (uint8_t n = 0; n < 16; ++n) {
        oled_display.textbox("CLEAR MD ", "LOCKS");

        mcl_seq.md_tracks[n].clear_locks();
      }
    } else if (opt_clear == 1) {
      oled_display.textbox("CLEAR ", "LOCKS");
      mcl_seq.md_tracks[last_md_track].clear_locks();
    }
  } else {
    auto &active_track = mcl_seq.ext_tracks[last_ext_track];
    if (opt_clear == 2) {
      oled_display.textbox("CLEAR ", "LOCKS");
      active_track.clear_track_locks();
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

void opt_copy_track_handler_cb() { opt_copy_track_handler(255); }

void opt_copy_track_handler(uint8_t op) {
  bool silent = false;
  opt_undo = 255;
  if (op != 255) {
    opt_copy = op;
    opt_undo = op;
    silent = true;
  }
  else {
    SET_BIT(copy_mask, (uint8_t) (opt_midi_device_capture == &MD) * 4 + opt_copy);
  }
  DEBUG_PRINTLN("copying");
  DEBUG_PRINTLN(opt_copy);
  DEBUG_PRINTLN(op);
  DEBUG_PRINTLN("/end");
  if (opt_copy == 2) {

    if (opt_midi_device_capture == &MD) {
      if (!silent) {
        oled_display.textbox("COPY MD ", "TRACKS");
        oled_display.display();
        MD.popup_text(1);
      }
      mcl_clipboard.copy_sequencer();
    }
#ifdef EXT_TRACKS
    else {
      if (!silent) {
        oled_display.textbox("COPY EXT ", "TRACKS");
        oled_display.display();
      }
      mcl_clipboard.copy_sequencer(NUM_MD_TRACKS);
    }
#endif
  }
  if (opt_copy == 1) {
    if (opt_midi_device_capture == &MD) {
      if (!silent) {
        oled_display.textbox("COPY TRACK", "");
        MD.popup_text(4);
      }
      mcl_clipboard.copy_track = last_md_track;
      mcl_clipboard.copy_sequencer_track(last_md_track);
    }
#ifdef EXT_TRACKS
    else {
      if (!silent) {
        const char *str = "COPY EXT TRACK";
        MD.popup_text((char*)str);
        oled_display.textbox(str,"");
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
  else if (!IS_BIT_SET(copy_mask, (uint8_t) (opt_midi_device_capture == &MD) * 4 + opt_paste)) {
    return;
  }
  if (opt_paste == 2) {

    if (opt_midi_device_capture == &MD) {
      if (!undo) {
        oled_display.textbox("PASTE MD ", "TRACKS");
        oled_display.display();
        MD.popup_text(3);
      } else {
        oled_display.textbox("UNDO ", "TRACKS");
        oled_display.display();
        MD.popup_text(22);
      }
      mcl_clipboard.paste_sequencer();
    } else {
      const char *str = "UNDO EXT TRACKS";
      if (!undo) {
        str = "PASTE EXT TRACKS";
      }
      oled_display.textbox(str, "");
      oled_display.display();
      MD.popup_text((char*)str);
      mcl_clipboard.paste_sequencer(NUM_MD_TRACKS);
    }
  }
  if (opt_paste == 1) {
    if (opt_midi_device_capture == &MD) {
      bool is_poly = false;
      if (!undo) {
        oled_display.textbox("PASTE TRACK", "");
        MD.popup_text(6);
      } else {
        oled_display.textbox("UNDO TRACK", "");
        is_poly = IS_BIT_SET16(mcl_cfg.poly_mask, last_md_track);
        MD.popup_text(23);
      }
      if (is_poly) {
        for (uint8_t c = 0; c < 16; c++) {
          if (IS_BIT_SET16(mcl_cfg.poly_mask, c)) {
            mcl_clipboard.paste_sequencer_track(c, c);
          }
        }
      }
      else {
      mcl_clipboard.paste_sequencer_track(mcl_clipboard.copy_track,
                                          last_md_track);
      }
    } else {
      const char *str = "UNDO EXT TRACK";
      if (!undo) {
        str = "PASTE EXT TRACK";
      }
      oled_display.textbox(str, "");
      MD.popup_text((char*)str);

      mcl_clipboard.paste_sequencer_track(mcl_clipboard.copy_track,
                                          last_ext_track + NUM_MD_TRACKS);
    }
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
  oled_display.textbox("CLEAR PAGE", "");
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

void opt_copy_page_handler_cb() {
  opt_copy_page_handler(255);
}

void opt_copy_page_handler(uint8_t op) {
  bool silent = false;
  opt_undo = 255;
  if (op != 255) {
    opt_undo = op;
    silent = true;
  }

  if (!silent) {
    oled_display.textbox("COPY PAGE", "");
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
    oled_display.textbox("PASTE PAGE", "");
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
  if (opt_undo != 255) {
    if (opt_undo != STEP_UNDO) {
      opt_undo = 255;
      goto CLEAR;
    }
    opt_paste_step_handler();
    return;
  } else {
  CLEAR:
    opt_copy_step_handler(STEP_UNDO);
  }
  char str[] = "CLEAR STEP";
  oled_display.textbox(str, "");
  MD.popup_text(str);
  MDSeqStep empty_step;
  memset(&empty_step, 0, sizeof(empty_step));
  mcl_seq.md_tracks[last_md_track].paste_step(
      SeqPage::step_select + SeqPage::page_select * 16, &empty_step);
  mcl_seq.md_tracks[last_md_track].clean_params();
}

void opt_copy_step_handler_cb() {
  opt_copy_step_handler(255);
}

void opt_copy_step_handler(uint8_t op) {
  bool silent = false;
  opt_undo = 255;
  if (op != 255) {
    opt_undo = op;
    silent = true;
  }
  if (!silent) {
    char str[] = "COPY STEP";
    oled_display.textbox(str, "");
    MD.popup_text(str);
  }
  mcl_seq.md_tracks[last_md_track].copy_step(SeqPage::step_select +
                                                 SeqPage::page_select * 16,
                                             &mcl_clipboard.steps[0]);
}

void opt_paste_step_handler() {
  if (opt_undo == STEP_UNDO) {
    opt_undo = 255;
    char str[] = "UNDO STEP";
    oled_display.textbox(str, "");
    MD.popup_text(str);
  } else {

    char str2[] = "PASTE STEP";
    oled_display.textbox(str2, "");
    MD.popup_text(str2);
  }
  mcl_seq.md_tracks[last_md_track].paste_step(SeqPage::step_select +
                                                  SeqPage::page_select * 16,
                                              &mcl_clipboard.steps[0]);
}

void opt_mute_step_handler() {
  for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {
    if (note_interface.is_note_on(n)) {
      TOGGLE_BIT64(mcl_seq.md_tracks[last_md_track].mute_mask,
                   n + SeqPage::page_select * 16);
    }
  }
}

void opt_clear_step_locks_handler() {
  if (opt_clear_step == 1) {
    oled_display.textbox("CLEAR STEP: ", "LOCKS");
    MD.popup_text(14);
  }
  for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {
    if (note_interface.is_note_on(n)) {

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
      // oled_display.textbox("REVERSE ", "MD TRACKS");
      for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {
        mcl_seq.md_tracks[n].reverse();
      }
    }
#ifdef EXT_TRACKS
    else {
      // oled_display.textbox("REVERSE ", "EXT TRACKS");
      for (uint8_t n = 0; n < NUM_EXT_TRACKS; n++) {
        mcl_seq.ext_tracks[n].reverse();
      }
    }
#endif
  }

  if (opt_reverse == 1) {
    if (opt_midi_device_capture == &MD) {
      // oled_display.textbox("REVERSE ", "TRACK");
      mcl_seq.md_tracks[last_md_track].reverse();
    }
#ifdef EXT_TRACKS
    else {
      // oled_display.textbox("REVERSE ", "EXT TRACK");
      mcl_seq.ext_tracks[last_ext_track].reverse();
    }
#endif
  }
}

void seq_menu_handler() {
}

void SeqPage::config_as_trackedit() {

  seq_menu_page.menu.enable_entry(SEQ_MENU_CLEAR_TRACK, true);
  seq_menu_page.menu.enable_entry(SEQ_MENU_CLEAR_LOCKS, false);
}

void SeqPage::config_as_lockedit() {

  seq_menu_page.menu.enable_entry(SEQ_MENU_CLEAR_TRACK, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_CLEAR_LOCKS, true);
}

bool SeqPage::md_track_change_check() {
  if (last_md_track != MD.currentTrack ||
      last_md_model != MD.kit.models[MD.currentTrack]) {
    last_md_model = MD.kit.models[MD.currentTrack];
    select_track(&MD, MD.currentTrack, false);
    return true;
  }
  return false;
}

void SeqPage::loop() {

  opt_midi_device_capture = midi_device;

  if (last_midi_state != MidiClock.state) {
    last_midi_state = MidiClock.state;
    DEBUG_DUMP("hii")
  }

  //  md_track_change_check();

  if (show_seq_menu) {
    seq_menu_page.loop();
    if (opt_midi_device_capture != &MD && opt_trackid > NUM_EXT_TRACKS) {
      // lock trackid to [1..4]
      opt_trackid = min(opt_trackid, NUM_EXT_TRACKS);
      seq_menu_value_encoder.cur = opt_trackid;
    }
    return;
  }
}

void SeqPage::draw_page_index(bool show_page_index, uint8_t _playing_idx) {
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
}

//  ref: design/Sequencer.png
void SeqPage::display() {

  bool is_md = (midi_device == &MD);
  const char *int_name = midi_active_peering.get_device(UART1_PORT)->name;
  const char *ext_name = midi_active_peering.get_device(UART2_PORT)->name;

  uint8_t track_id = last_md_track;
  if (!is_md) {
    track_id = last_ext_track;
  }
  track_id += 1;

  //  draw current active track
  mcl_gui.draw_panel_number(track_id);

  if (mcl.currentPage() == SEQ_EXTSTEP_PAGE) {
    mcl_gui.draw_panel_toggle(ext_name, int_name, true);
  }
  else {
    mcl_gui.draw_panel_toggle(int_name, ext_name, is_md);
  }
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
    if (show_seq_menu) {
      seq_menu_page.draw_menu(128 - width, 8, width);
    }
  }
}

void SeqPage::draw_knob_frame() {
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
