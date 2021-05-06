#include "MCL_impl.h"

#define MIDI_LOCAL_MODE 0
#define NUM_KEYS 24

scale_t *scales[24]{
    &chromaticScale, &ionianScale, &dorianScale, &phrygianScale, &lydianScale,
    &mixolydianScale, &aeolianScale, &locrianScale, &harmonicMinorScale,
    &melodicMinorScale,
    //&lydianDominantScale,
    //&wholeToneScale,
    //&wholeHalfStepScale,
    //&halfWholeStepScale,
    &majorPentatonicScale, &minorPentatonicScale, &suspendedPentatonicScale,
    &inSenScale, &bluesScale, &majorBebopScale, &dominantBebopScale,
    &minorBebopScale, &majorArp, &minorArp, &majorMaj7Arp, &majorMin7Arp,
    &minorMin7Arp,
    //&minorMaj7Arp,
    &majorMaj7Arp9,
    //&majorMaj7ArpMin9,
    //&majorMin7Arp9,
    //&majorMin7ArpMin9,
    //&minorMin7Arp9,
    //&minorMin7ArpMin9,
    //&minorMaj7Arp9,
    //&minorMaj7ArpMin9
};

typedef char scale_name_t[4];

const scale_name_t scale_names[] PROGMEM = {
    "---", "MAJ", "DOR", "PHR", "LYD", "MIX", "MIN", "LOC",
    "mHA", "mME", "MPE", "mPE", "sPE", "ISS", "BLU", "MBP",
    "DBP", "mBP", "MA",  "MIA", "MM7", "Mm7", "mm7", "M79",
};

void SeqPtcPage::setup() {
  SeqPage::setup();
  init_poly();
  midi_events.setup_callbacks();
  ptc_param_oct.cur = 1;
}
void SeqPtcPage::cleanup() {
  SeqPage::cleanup();
  // trig_interface.off();
  if (MidiClock.state != 2) {
    MD.setTrackParam(last_md_track, 0, MD.kit.params[last_md_track][0]);
  }
  //  midi_events.remove_callbacks();
}
void SeqPtcPage::config_encoders() {
  ptc_param_len.min = 1;
  bool show_chan = true;
  if (midi_device == &MD) {
    ptc_param_len.max = 64;
    ptc_param_len.cur = mcl_seq.md_tracks[last_md_track].length;
    show_chan = false;
  }
#ifdef EXT_TRACKS
  else {
    ptc_param_len.max = (uint8_t)128;
    ptc_param_len.cur = mcl_seq.ext_tracks[last_ext_track].length;
  }
#endif
  seq_menu_page.menu.enable_entry(SEQ_MENU_CHANNEL, show_chan);
}

uint8_t SeqPtcPage::calc_poly_count() {
  DEBUG_PRINT_FN();
  uint8_t count = 0;
  for (uint8_t x = 0; x < 16; x++) {
    if (IS_BIT_SET16(mcl_cfg.poly_mask, x)) {
      count++;
    }
  }
  DEBUG_PRINTLN(count);
  return count;
}

void SeqPtcPage::init_poly() {
  poly_count = 0;
  poly_max = calc_poly_count();
  for (uint8_t x = 0; x < 16; x++) {
    poly_notes[x] = -1;
  }
}

void SeqPtcPage::init() {
  DEBUG_PRINT_FN();
  SeqPage::init();
  seq_menu_page.menu.enable_entry(SEQ_MENU_TRACK, true);
  seq_menu_page.menu.enable_entry(SEQ_MENU_ARP, true);
  seq_menu_page.menu.enable_entry(SEQ_MENU_TRANSPOSE, true);
  seq_menu_page.menu.enable_entry(SEQ_MENU_POLY, true);
  cc_link_enable = true;
  ptc_param_len.handler = ptc_pattern_len_handler;
  note_mask = 0;
  DEBUG_PRINTLN(F("control mode:"));
  DEBUG_PRINTLN(mcl_cfg.uart2_ctrl_mode);
  trig_interface.on();
  trig_interface.send_md_leds(TRIGLED_EXCLUSIVE);
  if (mcl_cfg.uart2_ctrl_mode == MIDI_LOCAL_MODE) {
    trig_interface.on();
    note_interface.state = true;
  } else {
    trig_interface.off();
  }
  curpage = SEQ_PTC_PAGE;
  if (focus_track == 255) {
    focus_track = last_md_track;
  }
  config();
  re_init = false;
}

void SeqPtcPage::config() {
  config_encoders();
  ptc_param_finetune.cur = 32;

  // config info labels
  constexpr uint8_t len1 = sizeof(info1);
  char buf[len1] = {'\0'};

  char str_first[3] = "--";
  char str_second[3] = "--";
  if (midi_device == &MD) {
    const char *str;
    str = getMDMachineNameShort(MD.kit.get_model(last_md_track), 1);
    copyMachineNameShort(str, str_first);
    str = getMDMachineNameShort(MD.kit.get_model(last_md_track), 2);
    copyMachineNameShort(str, str_second);
  }
#ifdef EXT_TRACKS
  else {
    strcpy(str_first, midi_active_peering.get_device(UART2_PORT)->name);
    str_second[0] = 'T';
    str_second[1] = last_ext_track + '1';
  }
#endif
  strncpy(info1, str_first, len1);
  strncat(info1, ">", len1);
  strncat(info1, str_second, len1);

  strcpy(info2, "CHROMAT");
  display_page_index = false;

  // config menu
  config_as_trackedit();
}

void ptc_pattern_len_handler(EncoderParent *enc) {
  MCLEncoder *enc_ = (MCLEncoder *)enc;
  bool is_poly = IS_BIT_SET16(mcl_cfg.poly_mask, last_md_track);
  if (SeqPage::midi_device == &MD) {

    if (BUTTON_DOWN(Buttons.BUTTON3)) {
      for (uint8_t c = 0; c < 16; c++) {
        mcl_seq.md_tracks[c].set_length(enc_->cur);
      }
    } else {

      if ((seq_ptc_page.poly_max > 1) && (is_poly)) {
        for (uint8_t c = 0; c < 16; c++) {
          if (IS_BIT_SET16(mcl_cfg.poly_mask, c)) {
            mcl_seq.md_tracks[c].set_length(enc_->cur);
          }
        }
      } else {
        mcl_seq.md_tracks[last_md_track].set_length(enc_->cur);
      }
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

void SeqPtcPage::loop() {
  if (re_init) {
    init();
  }
#ifdef EXT_TRACKS
  if (ptc_param_oct.hasChanged() || ptc_param_scale.hasChanged()) {
    mcl_seq.ext_tracks[last_ext_track].buffer_notesoff();
    recalc_notemask();
    render_arp();
  }
#endif
  SeqPage::loop();
}

void SeqPtcPage::render_arp() {
  if (midi_device == &MD) {
    if (mcl_seq.md_tracks[last_md_track].speed == SEQ_SPEED_3_4X || mcl_seq.md_tracks[last_md_track].speed == SEQ_SPEED_3_2X) { 
       mcl_seq.md_arp_tracks[last_md_track].speed = SEQ_SPEED_3_2X;
    }
    else {
       mcl_seq.md_arp_tracks[last_md_track].speed = SEQ_SPEED_2X;
    }
    mcl_seq.md_arp_tracks[last_md_track].render(arp_mode.cur,arp_oct.cur, note_mask);
  }
  else {
     if (mcl_seq.md_tracks[last_ext_track].speed == SEQ_SPEED_3_4X || mcl_seq.ext_tracks[last_ext_track].speed == SEQ_SPEED_3_2X) {
       mcl_seq.md_tracks[last_ext_track].speed = SEQ_SPEED_3_2X;
     }
     else {
       mcl_seq.ext_arp_tracks[last_ext_track].speed = SEQ_SPEED_2X;
     } 
     mcl_seq.ext_arp_tracks[last_ext_track].render(arp_mode.cur, arp_oct.cur, note_mask);
  }
}

void SeqPtcPage::display() {
  if (!redisplay) {
    return;
  }

  oled_display.clearDisplay();
  auto *oldfont = oled_display.getFont();
 
  bool is_poly = IS_BIT_SET16(mcl_cfg.poly_mask, last_md_track);
  draw_knob_frame();
  char buf1[4];

  // draw OCTAVE
  itoa(ptc_param_oct.getValue(), buf1, 10);
  draw_knob(0, "OCT", buf1);

  // draw FREQ
  if (ptc_param_finetune.getValue() < 32) {
    strcpy(buf1, "-");
    itoa(32 - ptc_param_finetune.getValue(), buf1 + 1, 10);
  } else if (ptc_param_finetune.getValue() > 32) {
    strcpy(buf1, "+");
    itoa(ptc_param_finetune.getValue() - 32, buf1 + 1, 10);
  } else {
    strcpy(buf1, "0");
  }
  draw_knob(1, "DET", buf1); // detune

  // draw LEN
  if (midi_device == &MD) {
    itoa(ptc_param_len.getValue(), buf1, 10);
    if ((mcl_cfg.poly_mask > 0) && (is_poly)) {
      draw_knob(2, "PLEN", buf1);
    } else {
      draw_knob(2, "LEN", buf1);
    }
  }
#ifdef EXT_TRACKS
  else {
    itoa(ptc_param_len.getValue(), buf1, 10);
    draw_knob(2, "LEN", buf1);
  }
#endif

  // draw SCALE
  m_strncpy_p(buf1, scale_names[ptc_param_scale.getValue()], 4);
  draw_knob(3, "SCA", buf1);

  // draw TI keyboard
  mcl_gui.draw_keyboard(32, 23, 6, 9, NUM_KEYS, note_mask);

  oled_display.setFont(&TomThumb);
  oled_display.setCursor(107, 32);
  
  ArpSeqTrack *arp_track = &mcl_seq.ext_arp_tracks[last_ext_track];
  if (midi_device == &MD) {
    arp_track = &mcl_seq.md_arp_tracks[last_md_track];
  }
  if (arp_track->enabled) {
    oled_display.print("ARP");
  }
  else if ((mcl_cfg.poly_mask > 0) && (is_poly)) {
    oled_display.print("POLY");
  }
  SeqPage::display();
  oled_display.display();
  oled_display.setFont(oldfont);
}

uint8_t SeqPtcPage::calc_scale_note(uint8_t note_num) {
  uint8_t size = scales[ptc_param_scale.cur]->size;
  uint8_t oct = note_num / size;
  note_num = note_num - oct * size;

  return scales[ptc_param_scale.cur]->pitches[note_num] + oct * 12 + key;
}

uint8_t SeqPtcPage::get_next_voice(uint8_t pitch, uint8_t track_number) {
  uint8_t voice = 255;
  uint8_t count = 0;
  if (poly_max == 0 || (!IS_BIT_SET16(mcl_cfg.poly_mask, track_number))) {
    return track_number;
  }
  // If track previously played pitch, re-use this track
  for (uint8_t x = 0; x < 16; x++) {
    if (MD.isMelodicTrack(x) && IS_BIT_SET16(mcl_cfg.poly_mask, x)) {
      if (poly_notes[x] == pitch) {
        return x;
      }

      // Search for empty track.
      if (poly_notes[x] == -1) {
        voice = x;
      }
    }
  }
  // Reuse existing track for new pitch
  for (uint8_t x = 0; x < 16 && voice == 255; x++) {
    if (MD.isMelodicTrack(x) && IS_BIT_SET16(mcl_cfg.poly_mask, x)) {
      if (count == poly_count) {
        voice = x;
      } else {
        count++;
      }
    }
  }
  poly_count++;
  if ((poly_count >= 15) || (poly_count >= poly_max)) {
    poly_count = 0;
  }
  poly_notes[voice] = pitch;
  return voice;
}

uint8_t SeqPtcPage::get_machine_pitch(uint8_t track, uint8_t note_num) {
  tuning_t const *tuning = MD.getKitModelTuning(track);

  uint8_t note_offset = tuning->base - ((tuning->base / 12) * 12);
  note_num = note_num - note_offset;

  if (tuning == NULL) {
    return 255;
  }

  if (note_num >= tuning->len) {
    return 255;
  }

  uint8_t machine_pitch = max((int8_t) 0, (int8_t) pgm_read_byte(&tuning->tuning[note_num]) +
                          (int8_t)ptc_param_finetune.getValue() - 32);
  return min(machine_pitch,127);
}

void SeqPtcPage::trig_md(uint8_t note_num, uint8_t track_number, MidiUartParent *uart_) {
  if (track_number == 255) { track_number = last_md_track; }

  note_num = ptc_param_oct.cur * 12 + note_num;
  uint8_t next_track = get_next_voice(note_num, track_number);
  uint8_t machine_pitch = get_machine_pitch(next_track, note_num);
  if (machine_pitch == 255) {
    return;
  }
  MD.setTrackParam(next_track, 0, machine_pitch, uart_);
  MD.triggerTrack(next_track, 127, uart_);
  if ((recording) && (MidiClock.state == 2)) {

    mcl_seq.md_tracks[next_track].record_track(127);
    mcl_seq.md_tracks[next_track].record_track_pitch(machine_pitch);
  }
}

void SeqPtcPage::clear_trig_fromext(uint8_t note_num) {
  CLEAR_BIT64(note_mask, note_num);
  render_arp();
}

void SeqPtcPage::trig_md_fromext(uint8_t note_num) {
  uint8_t next_track = get_next_voice(note_num, last_md_track);
  uint8_t machine_pitch = get_machine_pitch(next_track, note_num);
  if (machine_pitch == 255) {
    return;
  }
  MD.setTrackParam(next_track, 0, machine_pitch);
  MD.triggerTrack(next_track, 127);
  if ((recording) && (MidiClock.state == 2)) {
    mcl_seq.md_tracks[next_track].record_track(127);
    mcl_seq.md_tracks[next_track].record_track_pitch(machine_pitch);
  }
}

void SeqPtcPage::note_on_ext(uint8_t note_num, uint8_t velocity, uint8_t track_number, MidiUartParent *uart_) {
    if (track_number == 255) { track_number = last_ext_track; }
    mcl_seq.ext_tracks[track_number].note_on(note_num, velocity, uart_);
    if ((seq_ptc_page.recording) && (MidiClock.state == 2)) {
      mcl_seq.ext_tracks[track_number].record_track_noteon(note_num, velocity);
    } 
}
void SeqPtcPage::note_off_ext(uint8_t note_num, uint8_t velocity, uint8_t track_number, MidiUartParent *uart_) {
  if (track_number == 255) { track_number = last_ext_track; }
  mcl_seq.ext_tracks[last_ext_track].note_off(note_num, uart_);
  if (seq_ptc_page.recording && (MidiClock.state == 2)) {
    mcl_seq.ext_tracks[last_ext_track].record_track_noteoff(note_num);
  }
}

void SeqPtcPage::recalc_notemask() {
  note_mask = 0;
  for (uint8_t i = 0; i < 24; i++) {
    if (note_interface.is_note_on(i)) {
      uint8_t pitch = calc_scale_note(i);
      SET_BIT64(note_mask, pitch);
    }
  }
}

bool SeqPtcPage::handleEvent(gui_event_t *event) {

  if (SeqPage::handleEvent(event)) {
    if (show_seq_menu) {
      redisplay = true;
      return true;
    }
    queue_redraw();
  }

  bool is_poly = IS_BIT_SET16(mcl_cfg.poly_mask, last_md_track);

  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    auto device = midi_active_peering.get_device(port);

    // do not route EXT TI events to MD.
    if (device != &MD) {
      return false;
    }

    uint8_t note = event->source - 128;
    uint8_t pitch = calc_scale_note(note);
    DEBUG_PRINTLN(F("yep"));
    // note interface presses are treated as musical notes here
    if (mask == EVENT_BUTTON_PRESSED) {

      SET_BIT64(note_mask, pitch);
      render_arp();
      if (midi_device != &MD) {
        midi_device = device;
        config();
      } else {
        focus_track = last_md_track;
        config_encoders();
      }
      arp_page.track_update();
      midi_device = &MD;

      ArpSeqTrack *arp_track = &mcl_seq.md_arp_tracks[last_md_track];
      if ((!arp_track->enabled) || (MidiClock.state != 2)) {
        trig_md(pitch);
      }
    } else if (mask == EVENT_BUTTON_RELEASED) {
      if (arp_und.cur != ARP_LATCH) {
        CLEAR_BIT64(note_mask, pitch);
        render_arp();
      }
    }

    trig_interface.send_md_leds(TRIGLED_EXCLUSIVE);
    // deferred trigger redraw to update TI keyboard feedback.
    queue_redraw();

    return true;
  } // TI events

  if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    if (BUTTON_DOWN(Buttons.BUTTON4)) {
      re_init = true;
      GUI.pushPage(&poly_page);
      return true;
    }
    queue_redraw();
    mcl_seq.ext_tracks[last_ext_track].init_notes_on();

    recording = !recording;
    if (recording) {
      MD.set_rec_mode(2);
      setLed2();
      oled_display.textbox("REC", "");
    }
    else {
      MD.set_rec_mode(1);
      clearLed2();
    }
    return true;
  }
  /*
    if (EVENT_PRESSED(event, Buttons.ENCODER4)) {
      GUI.setPage(&grid_page);
      return true;
    }
  */

  if (EVENT_RELEASED(event, Buttons.BUTTON4)) {
    if (BUTTON_DOWN(Buttons.BUTTON1)) {
      re_init = true;
      GUI.pushPage(&poly_page);
      return true;
    }
    if (midi_device == &MD) {

      if ((poly_max > 1) && (is_poly)) {
#ifdef OLED_DISPLAY
        oled_display.textbox("CLEAR ", "POLY TRACKS");
#endif
        for (uint8_t c = 0; c < 16; c++) {
          if (IS_BIT_SET16(mcl_cfg.poly_mask, c)) {
            mcl_seq.md_tracks[c].clear_track();
          }
        }
      } else {
#ifdef OLED_DISPLAY
        oled_display.textbox("CLEAR ", "TRACK");
#endif
        mcl_seq.md_tracks[last_md_track].clear_track();
      }
    }
#ifdef EXT_TRACKS
    else {
#ifdef OLED_DISPLAY
      oled_display.textbox("CLEAR ", "EXT TRACK");
#endif
      mcl_seq.ext_tracks[last_ext_track].clear_track();
    }
#endif
    return true;
  }

  return false;
}

#define NOTE_C2 48

uint8_t SeqPtcPage::seq_ext_pitch(uint8_t note_num, MidiDevice* device) {

  uint8_t oct = 0;
  uint8_t note = note_num;
  if (device == &MD) {
    note = note_num - (note_num / 12) * 12;
    if (note_num >= NOTE_C2) {
      oct = (note_num / 12) - (NOTE_C2 / 12);
    } else {
    return 255;
    }
  }
  return calc_scale_note(note + oct * 12);
}


uint8_t process_ext_pitch(uint8_t note_num, bool note_type, MidiDevice* device) {
  uint8_t pitch = seq_ptc_page.seq_ext_pitch(note_num, device);
  if (pitch == 255) { return 255; }

  uint8_t scaled_pitch = pitch - (pitch / 24) * 24;
  if (note_type) {
    SET_BIT64(seq_ptc_page.note_mask, scaled_pitch);
  }
  else {
    CLEAR_BIT64(seq_ptc_page.note_mask, scaled_pitch);
  }
  pitch += ptc_param_oct.cur * 12;
  return pitch;
}

void SeqPtcMidiEvents::onNoteOnCallback_Midi2(uint8_t *msg) {
  if ((GUI.currentPage() == &seq_step_page) ||
#ifdef EXT_TRACKS
      (GUI.currentPage() == &seq_extstep_page) ||
#endif
      (GUI.currentPage() == &grid_save_page) ||
      (GUI.currentPage() == &grid_write_page)) {
    return;
  }
  uint8_t note_num = msg[1];
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  DEBUG_PRINTLN("note on");
  DEBUG_DUMP(channel);

  // matches control channel, or MIDI2 is OMNI?
  // then route midi message to MD
  //

  // pitch - MIDI_NOTE_C4
  //
  uint8_t pitch;
  bool note_on = true;
  if ((mcl_cfg.uart2_ctrl_mode - 1 == channel) ||
      (mcl_cfg.uart2_ctrl_mode == MIDI_OMNI_MODE)) {

    SeqPage::midi_device = midi_active_peering.get_device(UART1_PORT);
    pitch = process_ext_pitch(note_num, note_on, SeqPage::midi_device);
    if (pitch == 255) return;

    if (GUI.currentPage() == &seq_ptc_page) {
      seq_ptc_page.focus_track = last_md_track;
    }

    ArpSeqTrack *arp_track = &mcl_seq.md_arp_tracks[last_md_track];

    seq_ptc_page.render_arp();
    arp_page.track_update();
    seq_ptc_page.queue_redraw();
 
    if (!arp_track->enabled) {
      seq_ptc_page.trig_md_fromext(pitch);
      
    }
    
    return;
  }
  if (channel >= mcl_seq.num_ext_tracks) {
    return;
  }
#ifdef EXT_TRACKS
  // otherwise, translate the message and send it back to MIDI2.
  auto active_device = midi_active_peering.get_device(UART2_PORT);
  
  if (SeqPage::midi_device != active_device || (mcl_seq.ext_tracks[last_ext_track].channel != channel)) {

    SeqPage::midi_device = active_device;
    seq_ptc_page.set_last_ext_track(channel);
    seq_ptc_page.config();
  } else {
    SeqPage::midi_device = active_device;
  }
  pitch = process_ext_pitch(note_num, note_on, SeqPage::midi_device);
  seq_ptc_page.set_last_ext_track(channel);
  seq_ptc_page.config_encoders();

  DEBUG_PRINTLN(mcl_seq.ext_tracks[last_ext_track].length);
  DEBUG_PRINTLN(F("Sending note"));
  DEBUG_DUMP(pitch);
  
  ArpSeqTrack *arp_track = &mcl_seq.ext_arp_tracks[last_ext_track];
  seq_ptc_page.render_arp();
  arp_page.track_update();
  seq_ptc_page.queue_redraw();
 
  if (!arp_track->enabled) {
     seq_ptc_page.note_on_ext(pitch, msg[2]);
  }
#endif
  return;
}

void SeqPtcPage::set_last_ext_track(uint8_t channel) {
  for (uint8_t n = 0; n < NUM_EXT_TRACKS; n++) {
    if (mcl_seq.ext_tracks[n].channel == channel) {
     last_ext_track = n;
     break;
    }
  }
}

void SeqPtcMidiEvents::onNoteOffCallback_Midi2(uint8_t *msg) {
  if ((GUI.currentPage() == &seq_step_page) ||
#ifdef EXT_TRACKS
      (GUI.currentPage() == &seq_extstep_page) ||
#endif
      (GUI.currentPage() == &grid_save_page) ||
      (GUI.currentPage() == &grid_write_page)) {
    return;
  }
  DEBUG_PRINTLN(F("note off midi2"));
  uint8_t note_num = msg[1];
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t pitch;

  bool note_on = false;
  if ((mcl_cfg.uart2_ctrl_mode - 1 == channel) ||
      (mcl_cfg.uart2_ctrl_mode == MIDI_OMNI_MODE)) {

    if (arp_und.cur != ARP_LATCH) {
      pitch = process_ext_pitch(note_num, note_on, SeqPage::midi_device);
      if (pitch == 255) return;
      seq_ptc_page.render_arp();
    }
    else {
      note_on = false;
      pitch = process_ext_pitch(note_num, note_on, SeqPage::midi_device);
      if (pitch == 255) return;
    }
    seq_ptc_page.clear_trig_fromext(pitch);
    seq_ptc_page.queue_redraw();
    return;
  }

#ifdef EXT_TRACKS
  SeqPage::midi_device = midi_active_peering.get_device(UART2_PORT);
  pitch = process_ext_pitch(note_num, note_on, SeqPage::midi_device);
  if (channel >= mcl_seq.num_ext_tracks) {
    return;
  }
  seq_ptc_page.set_last_ext_track(channel);
  seq_ptc_page.config_encoders();

  seq_ptc_page.render_arp();
  arp_page.track_update();
  seq_ptc_page.queue_redraw();

  ArpSeqTrack *arp_track = &mcl_seq.md_arp_tracks[last_md_track];
  if (!arp_track->enabled) {
     seq_ptc_page.note_off_ext(pitch, msg[2]);
  }

#endif
}

void SeqPtcMidiEvents::onControlChangeCallback_Midi2(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
  uint8_t track;
  uint8_t track_param;
  // If external keyboard controlling MD param, send parameter updates
  // to all polyphonic tracks
  if ((param < 16) || (param > 39)) {
    return;
  }
  // If Midi2 forwarding data to port 1 , ignore this to prevent double
  // messages.
  //
  if (mcl_cfg.midi_forward == 2) {
    return;
  }
  if ((mcl_cfg.uart2_ctrl_mode - 1 == channel) ||
      (mcl_cfg.uart2_ctrl_mode == MIDI_OMNI_MODE)) {
    for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {
      if (IS_BIT_SET16(mcl_cfg.poly_mask, n)) {
        MD.setTrackParam(n, param - 16, value);
      }
    }
  }
}
void SeqPtcMidiEvents::onControlChangeCallback_Midi(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
  uint8_t track;
  uint8_t track_param;
  uint8_t display_polylink = 0;

  if (!seq_ptc_page.cc_link_enable) { return; }

  MD.parseCC(channel, param, &track, &track_param);
  uint8_t start_track;
  if (track_param == 32) { return; } //don't process mute
  if ((seq_ptc_page.poly_max > 1)) {
    if (IS_BIT_SET16(mcl_cfg.poly_mask, track)) {

      for (uint8_t n = 0; n < 16; n++) {

        if (IS_BIT_SET16(mcl_cfg.poly_mask, n) && (n != track)) {
          if (track_param < 24) {
            MD.setTrackParam(n, track_param, value);
            display_polylink = 1;
         }
        }
        // in_sysex = 0;
      }
    }
  }

  if (display_polylink && GUI.currentPage() != &mixer_page) {
    oled_display.textbox("POLY-", "LINK");
    seq_ptc_page.queue_redraw();
  }

}

void SeqPtcMidiEvents::setup_callbacks() {
  if (state) {
    return;
  }
  Midi2.addOnNoteOnCallback(
      this, (midi_callback_ptr_t)&SeqPtcMidiEvents::onNoteOnCallback_Midi2);
  Midi2.addOnNoteOffCallback(
      this, (midi_callback_ptr_t)&SeqPtcMidiEvents::onNoteOffCallback_Midi2);
  Midi.addOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&SeqPtcMidiEvents::onControlChangeCallback_Midi);
  Midi2.addOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&SeqPtcMidiEvents::onControlChangeCallback_Midi2);
  state = true;
}

void SeqPtcMidiEvents::remove_callbacks() {
  if (!state) {
    return;
  }

  DEBUG_PRINTLN(F("remove calblacks"));
  Midi2.removeOnNoteOnCallback(
      this, (midi_callback_ptr_t)&SeqPtcMidiEvents::onNoteOnCallback_Midi2);
  Midi2.removeOnNoteOffCallback(
      this, (midi_callback_ptr_t)&SeqPtcMidiEvents::onNoteOffCallback_Midi2);
  Midi.removeOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&SeqPtcMidiEvents::onControlChangeCallback_Midi);
  Midi.removeOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&SeqPtcMidiEvents::onControlChangeCallback_Midi2);

  state = false;
}
