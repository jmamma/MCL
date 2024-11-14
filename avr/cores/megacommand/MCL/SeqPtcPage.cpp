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
  octs[0] = 0;
  octs[1] = 1;
  fine_tunes[0] = 32;
  fine_tunes[1] = 32;
  memset(dev_note_masks, 0, sizeof(dev_note_masks));
  memset(dev_note_channels, 17, sizeof(dev_note_channels));
  memset(note_mask, 0, sizeof(note_mask));
  config_encoders();
  isSetup = true;
}
void SeqPtcPage::cleanup() {
  SeqPage::cleanup();
  last_midi_device = midi_device;
  params_reset();
}
void SeqPtcPage::config_encoders() {
  if (show_seq_menu) { return; }
  ptc_param_len.min = 1;
  bool show_chan = true;

  uint8_t dev = midi_device == &MD ? 0 : 1;

  encoders[0]->cur = octs[dev];
  encoders[1]->cur = fine_tunes[dev];

  if (dev == 0) {
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

void SeqPtcPage::init_poly() {
  for (uint8_t x = 0; x < 16; x++) {
    poly_notes[x] = -1;
    poly_order[x] = 0;
  }
  cc_link_enable = true;
}

void SeqPtcPage::init() {
  DEBUG_PRINT_FN();
  if (last_midi_device != nullptr) { midi_device = last_midi_device; }
  SeqPage::init();
  seq_menu_page.menu.enable_entry(SEQ_MENU_DEVICE, true);
  seq_menu_page.menu.enable_entry(SEQ_MENU_TRACK, true);
  seq_menu_page.menu.enable_entry(SEQ_MENU_ARP, true);
  seq_menu_page.menu.enable_entry(SEQ_MENU_KEY, true);
  seq_menu_page.menu.enable_entry(SEQ_MENU_POLY, true);
  if (midi_device == &MD) {
    seq_menu_page.menu.enable_entry(SEQ_MENU_SOUND, true);
    seq_menu_page.menu.enable_entry(SEQ_MENU_LENGTH_MD, true);
  }
  else {
    seq_menu_page.menu.enable_entry(SEQ_MENU_LENGTH_EXT, true);
  }
  cc_link_enable = true;
  scale_padding = false;
  ptc_param_len.handler = pattern_len_handler;
  DEBUG_PRINTLN(F("control mode:"));
  DEBUG_PRINTLN(mcl_cfg.uart2_ctrl_chan);
  trig_interface.on();
  trig_interface.send_md_leds(TRIGLED_EXCLUSIVE);
  config();
  re_init = false;
}

void SeqPtcPage::config() {
  config_encoders();
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
void SeqPtcPage::loop() {
  if (re_init) {
    init();
  }
  if (ptc_param_oct.hasChanged() || ptc_param_scale.hasChanged() ||
      ptc_param_fine_tune.hasChanged()) {
    uint8_t dev = midi_device == &MD ? 0 : 1;
    octs[dev] = encoders[0]->cur;
    fine_tunes[dev] = encoders[1]->cur;

    uint8_t track = last_md_track;
    if (dev) {
      track = last_ext_track;
      mcl_seq.ext_tracks[last_ext_track].buffer_notesoff();
    }
    render_arp(ptc_param_scale.hasChanged(), midi_device, track);
  }
  SeqPage::loop();
}
uint8_t SeqPtcPage::find_arp_track(uint8_t channel_event) {
  uint8_t track = last_md_track;
  if (IS_BIT_SET16(mcl_cfg.poly_mask, last_md_track)) { channel_event = POLY_EVENT; }
  if (channel_event == POLY_EVENT) {
    uint16_t mask = mcl_cfg.poly_mask;
    uint8_t n = 0;
    while (mask) {
      if (mask & 1) {
        return n;
      }
      n++;
      mask = mask >> 1;
    }
  }
  return track;
}

void SeqPtcPage::render_arp(bool recalc_notemask_, MidiDevice *midi_dev,
                            uint8_t track) {
  if (recalc_notemask_) {
    recalc_notemask();
  }

  SeqTrack *seq_track;
  ArpSeqTrack *arp_track;
  if (midi_dev == &MD) {
    seq_track = &mcl_seq.md_tracks[track];
    arp_track = &mcl_seq.md_arp_tracks[track];
  } else {
    seq_track = &mcl_seq.ext_tracks[track];
    arp_track = &mcl_seq.ext_arp_tracks[track];
  }
  if (seq_track->speed == SEQ_SPEED_3_4X ||
      seq_track->speed == SEQ_SPEED_3_2X) {
    arp_track->speed = SEQ_SPEED_3_2X;
  } else {
    arp_track->speed = SEQ_SPEED_2X;
  }
  arp_track->render(arp_mode.cur, ptc_param_oct.cur, ptc_param_fine_tune.cur,
                    arp_range.cur, note_mask);
}

void SeqPtcPage::display() {

  oled_display.clearDisplay();
  bool is_poly = IS_BIT_SET16(mcl_cfg.poly_mask, last_md_track);
  draw_knob_frame();
  char buf1[4];

  // draw OCTAVE
  mcl_gui.put_value_at(ptc_param_oct.getValue(), buf1);
  draw_knob(0, "OCT", buf1);

  // draw FREQ
  if (ptc_param_fine_tune.getValue() < 32) {
    strcpy(buf1, "-");
    mcl_gui.put_value_at(32 - ptc_param_fine_tune.getValue(), buf1 + 1);
  } else if (ptc_param_fine_tune.getValue() > 32) {
    strcpy(buf1, "+");
    mcl_gui.put_value_at(ptc_param_fine_tune.getValue() - 32, buf1 + 1);
  } else {
    strcpy(buf1, "0");
  }
  draw_knob(1, "DET", buf1); // detune

  // draw LEN
  if (midi_device == &MD) {
    mcl_gui.put_value_at(ptc_param_len.getValue(), buf1);
    if ((mcl_cfg.poly_mask > 0) && (is_poly)) {
      draw_knob(2, "PLEN", buf1);
    } else {
      draw_knob(2, "LEN", buf1);
    }
  }
#ifdef EXT_TRACKS
  else {
    mcl_gui.put_value_at(ptc_param_len.getValue(), buf1);
    draw_knob(2, "LEN", buf1);
  }
#endif

  // draw SCALE
  strncpy_P(buf1, scale_names[ptc_param_scale.getValue()], 4);
  draw_knob(3, "SCA", buf1);

  // draw TI keyboard

  oled_display.setFont(&TomThumb);
  oled_display.setCursor(105, 32);

  ArpSeqTrack *arp_track = &mcl_seq.ext_arp_tracks[last_ext_track];
  if (midi_device == &MD) {
    arp_track = &mcl_seq.md_arp_tracks[last_md_track];
  }
  if ((mcl_cfg.poly_mask > 0) && (is_poly)) {
    oled_display.print(F("PLY"));
  }

  uint64_t *mask = note_mask;
  if (arp_track->enabled) {
    oled_display.print(F("ARP"));
    mask = arp_track->note_mask;
  }

  mcl_gui.draw_keyboard(32, 23, 6, 9, NUM_KEYS, mask);
  SeqPage::display();
  if (show_seq_menu) {
    display_mute_mask(midi_active_peering.get_device(UART2_PORT), 8);
  }
}

uint8_t SeqPtcPage::calc_scale_note(uint8_t note_num, bool padded) {
  uint8_t size = scales[ptc_param_scale.cur]->size;
  uint8_t oct;

  uint8_t d = size;
  if (padded) {
    d = 12;
  }
  oct = note_num / d;
  note_num = note_num - oct * d;

  uint8_t pos = note_num;

  if (padded) {
    // pos = note_num - (note_num / (size + 1)) * (size + 1);
    // pos = min(note_num, size);
    const uint16_t chromatic = 0b0000010101001010;
    if (IS_BIT_SET16(chromatic, note_num)) {
      note_num--;
    }
    if (size < 12) {
      pos = round((float)(size - 1) * (float)note_num / 12.0);
    }
  }

  return scales[ptc_param_scale.cur]->pitches[pos] + oct * 12 + transpose;
}

uint8_t SeqPtcPage::get_next_voice(uint8_t pitch, uint8_t track_number,
                                   uint8_t channel_event) {
  uint8_t voice = 255;

  if (channel_event == POLY_EVENT) {
    goto poly;
  } else if (channel_event == CTRL_EVENT) {

    // mono
    if (!mcl_cfg.poly_mask ||
        (!IS_BIT_SET16(mcl_cfg.poly_mask, track_number))) {
      return track_number;
    }
  } else {
    return 255;
  }

poly:
  // If track previously played pitch, re-use this track
  for (uint8_t x = 0; x < 16; x++) {
    if (MD.isMelodicTrack(x) && IS_BIT_SET16(mcl_cfg.poly_mask, x)) {
      if (poly_notes[x] == pitch || poly_notes[x] == -1) {
        voice = x;
      }
    }
  }

  int oldest_val = -1;
  if (voice != 255) {
    goto end;
  }
  // Reuse oldest note

  for (uint8_t x = 0; x < 16; x++) {
    if (MD.isMelodicTrack(x) && IS_BIT_SET16(mcl_cfg.poly_mask, x)) {
      if (poly_order[x] > oldest_val) {
        voice = x;
        oldest_val = poly_order[x];
      }
    }
  }

  // Shift up

end:

  for (uint8_t x = 0; x < 16; x++) {
    if (MD.isMelodicTrack(x) && IS_BIT_SET16(mcl_cfg.poly_mask, x)) {
      if (poly_order[x] <= poly_order[voice] && x != voice) {
        poly_order[x]++;
      }
    }
  }
  // set selected voice to be the latest note.

  poly_order[voice] = 0;
  poly_notes[voice] = pitch;

  return voice;
}

uint8_t SeqPtcPage::get_note_from_machine_pitch(uint8_t track_number, uint8_t pitch) {
  uint8_t note_num = 255;
  bool is_midi_model = ((MD.kit.models[track_number] & 0xF0) == MID_01_MODEL);
  if (is_midi_model) { return pitch; }

  tuning_t const *tuning = MD.getKitModelTuning(track_number);
  pitch -= ptc_param_fine_tune.getValue() - 32;
  if (pitch != 255 && tuning) {
    for (uint8_t i = 0; i < tuning->len; i++) {
      uint8_t ccStored = pgm_read_byte(&tuning->tuning[i]);
      if (ccStored >= pitch) {
        note_num = i;
        break;
      }
    }
    uint8_t note_offset = tuning->base - ((tuning->base / 12) * 12);
    return note_num + note_offset;
  }
  return 255;
}

uint8_t SeqPtcPage::get_machine_pitch(uint8_t track, uint8_t note_num,
                                      uint8_t fine_tune) {
  if (fine_tune == 255) {
    fine_tune = ptc_param_fine_tune.getValue();
  }

  tuning_t const *tuning = MD.getKitModelTuning(track);

  uint8_t note_offset = tuning->base - ((tuning->base / 12) * 12);
  note_num = note_num - note_offset;

  if (tuning == NULL) {
    return 255;
  }

  if (note_num >= tuning->len) {
    return 255;
  }

  uint8_t machine_pitch =
      max((int8_t)0, (int8_t)pgm_read_byte(&tuning->tuning[note_num]) +
                         (int8_t)fine_tune - 32);
  return min(machine_pitch, 127);
}

void SeqPtcPage::trig_md(uint8_t note_num, uint8_t track_number, uint8_t channel_event,  uint8_t fine_tune, MidiUartParent *uart_) {
  if (track_number == 255) {
    track_number = last_md_track;
  }

  uint8_t next_track = get_next_voice(note_num, track_number, channel_event);
  if (next_track > 15) { return; }

  uint8_t machine_pitch = get_machine_pitch(next_track, note_num, fine_tune);
  bool is_midi_model_ = ((MD.kit.models[track_number] & 0xF0) == MID_01_MODEL);
  if (is_midi_model_) {
    machine_pitch = note_num;
    next_track = track_number;
    mcl_seq.md_tracks[next_track].send_notes_off();
    mcl_seq.md_tracks[next_track].send_notes(machine_pitch);
    goto rec;
  }
  if (machine_pitch == 255) {
    return;
  }

  MD.setTrackParam(next_track, 0, machine_pitch, uart_);
  MD.triggerTrack(next_track, 127, uart_);
  mixer_page.trig(next_track);
  rec:
  record(machine_pitch, next_track);
}

void SeqPtcPage::record(uint8_t pitch, uint8_t track) {
  if ((recording) && (MidiClock.state == 2)) {
    reset_undo();
    mcl_seq.md_tracks[track].record_track(127);
    mcl_seq.md_tracks[track].record_track_pitch(pitch);
  }

}
void SeqPtcPage::note_on_ext(uint8_t note_num, uint8_t velocity,
                             uint8_t track_number, MidiUartParent *uart_) {
  if (track_number == 255) {
    track_number = last_ext_track;
  }
  mcl_seq.ext_tracks[track_number].note_on(note_num, velocity, uart_);
  // if ((seq_ptc_page.recording) && (MidiClock.state == 2)) {
  reset_undo();
  mcl_seq.ext_tracks[track_number].record_track_noteon(note_num, velocity);
  //}
}

void SeqPtcPage::note_off_ext(uint8_t note_num, uint8_t velocity,
                              uint8_t track_number, MidiUartParent *uart_) {
  if (track_number == 255) {
    track_number = last_ext_track;
  }
  mcl_seq.ext_tracks[track_number].note_off(note_num, velocity, uart_);
  // if (seq_ptc_page.recording && (MidiClock.state == 2)) {
  reset_undo();
  mcl_seq.ext_tracks[track_number].record_track_noteoff(note_num);
  //}
}

void SeqPtcPage::buffer_notesoff_ext(uint8_t track_number) {
  mcl_seq.ext_tracks[track_number].buffer_notesoff();
}

void SeqPtcPage::recalc_notemask() {
  memset(note_mask, 0, sizeof(note_mask));

  uint8_t dev = (midi_device == &MD) ? 0 : 1;

  for (uint8_t i = 0; i < 128; i++) {
    if (IS_BIT_SET128_P(dev_note_masks[dev], i)) {
      uint8_t pitch = calc_scale_note(i, scale_padding);
      if (pitch > 127)
        continue;
      SET_BIT128_P(note_mask, pitch);
    }
  }
}

void SeqPtcPage::draw_popup_transpose() {
  char str[] = "KEY:   ";
  strcpy(str + 5, number_to_note.notes_upper[transpose]);
  MD.popup_text(str);
  oled_display.textbox(str, "");
}

void SeqPtcPage::draw_popup_octave() {
  char str[] = "OCT:   ";
  mcl_gui.put_value_at(ptc_param_oct.cur, str + 5);
  MD.popup_text(str);
}

bool SeqPtcPage::handleEvent(gui_event_t *event) {

  bool is_poly = IS_BIT_SET16(mcl_cfg.poly_mask, last_md_track);

  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    auto device = midi_active_peering.get_device(port);
    uint8_t note = event->source - 128;
    // do not route EXT TI events to MD.
    if (device != &MD) {
      return false;
    }
    if (show_seq_menu) {
      if (mask == EVENT_BUTTON_PRESSED) {
        toggle_ext_mask(note);
      }
      return true;
    }
    /*
    if (mask == EVENT_BUTTON_PRESSED) {
      SET_BIT128_P(dev_note_masks[0], note);
    } else {
      CLEAR_BIT128_P(dev_note_masks[0], note);
    }
    */

    // note interface presses are treated as musical notes here
    scale_padding = false;
    bool is_md = midi_device == &MD;
    uint8_t channel_event = NO_EVENT;

    if (is_md) {
      note += MIDI_NOTE_C4;
      bool is_poly = IS_BIT_SET16(mcl_cfg.poly_mask, last_md_track);
      channel_event = is_poly ? POLY_EVENT : CTRL_EVENT;
    } else {
//      note += MIDI_NOTE_C1;
    }
    uint8_t msg[] = {MIDI_NOTE_ON | (is_md ? last_md_track : last_ext_track),
                     note, 127};

    if (mask == EVENT_BUTTON_PRESSED) {
      midi_events.note_on(msg, channel_event);

    } else if (mask == EVENT_BUTTON_RELEASED) {
      midi_events.note_off(msg, channel_event);
    }

    trig_interface.send_md_leds(TRIGLED_EXCLUSIVE);
    // deferred trigger redraw to update TI keyboard feedback.

    return true;
  } // TI events

  if (EVENT_CMD(event)) {
    uint8_t key = event->source - 64;
    if (trig_interface.is_key_down(MDX_KEY_PATSONG)) {
      return seq_menu_page.handleEvent(event);
    }
    if (event->mask == EVENT_BUTTON_PRESSED &&
        !trig_interface.is_key_down(MDX_KEY_FUNC)) {
      switch (key) {
      case MDX_KEY_LEFT: {
        if (transpose > 0) {
          transpose -= 1;
        }
        draw_popup_transpose();
        return true;
      }
      case MDX_KEY_RIGHT: {
        if (transpose < 11) {
          transpose += 1;
        }
        draw_popup_transpose();
        return true;
      }
      case MDX_KEY_UP: {
        if (ptc_param_oct.cur < 8) {
          ptc_param_oct.cur += 1;
        }
        draw_popup_octave();
        return true;
      }
      case MDX_KEY_DOWN: {
        if (ptc_param_oct.cur > 0) {
          ptc_param_oct.cur -= 1;
        }
        draw_popup_octave();
        return true;
      }
      case MDX_KEY_SCALE: {
        midi_device = midi_device == &MD
                          ? midi_active_peering.get_device(UART2_PORT)
                          : midi_active_peering.get_device(UART1_PORT);
        config();
        return true;
      }
      }
    }
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    if (BUTTON_DOWN(Buttons.BUTTON4)) {
      re_init = true;
      mcl.pushPage(POLY_PAGE);
      return true;
    }
    mcl_seq.ext_tracks[last_ext_track].init_notes_on();
    toggle_record();
    return true;
  }
  /*
    if (EVENT_PRESSED(event, Buttons.ENCODER4)) {
      mcl.setPage(GRID_PAGE);
      return true;
    }
  */
  if (EVENT_RELEASED(event, Buttons.BUTTON4)) {
    if (BUTTON_DOWN(Buttons.BUTTON1)) {
      re_init = true;
      mcl.pushPage(POLY_PAGE);
      return true;
    }
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
    mute_mask = 128;
  }
  if (SeqPage::handleEvent(event)) {
    return true;
  }

  return false;
}

uint8_t SeqPtcPage::seq_ext_pitch(uint8_t note_num) {
  uint8_t pitch = calc_scale_note(note_num, scale_padding);
  return (pitch < 128) ? pitch : 255;
}

uint8_t SeqPtcPage::process_ext_event(uint8_t note_num, bool note_type,
                                      uint8_t channel) {

  uint8_t pitch = seq_ptc_page.seq_ext_pitch(note_num);
  uint8_t dev = (midi_device == &MD) ? 0 : 1;

  SeqTrackBase *arp_track = dev ? (SeqTrackBase*) &mcl_seq.ext_arp_tracks[last_ext_track] : (SeqTrackBase*) &mcl_seq.md_arp_tracks[last_md_track];
  dev_note_channels[dev] = channel;
  if (note_type) {
    bool notes_all_off = seq_ptc_page.dev_note_masks[dev][0] == 0 && seq_ptc_page.dev_note_masks[dev][1] == 0;

    if (notes_all_off) {
      if (dev) { mcl_seq.ext_arp_tracks[last_ext_track].idx = 0; }
      else { mcl_seq.md_arp_tracks[last_md_track].idx = 0; }

      if (mcl_cfg.rec_quant == 0) {
        arp_track->mod12_counter = arp_track->get_timing_mid() - 2;
        arp_track->step_count = arp_track->length - 1;
      }
      if (arp_enabled.cur == ARP_LATCH) {
        memset(seq_ptc_page.note_mask, 0, sizeof(seq_ptc_page.note_mask));
      }
    }
    SET_BIT128_P(seq_ptc_page.dev_note_masks[dev], note_num);
    if (pitch != 255) {
      SET_BIT128_P(seq_ptc_page.note_mask, pitch);
    }
  } else {
    CLEAR_BIT128_P(seq_ptc_page.dev_note_masks[dev], note_num);
    if (arp_enabled.cur != ARP_LATCH && pitch != 255) {
      CLEAR_BIT128_P(seq_ptc_page.note_mask, pitch);
    }
  }
  if (pitch == 255) {
    return 255;
  }
  pitch += ptc_param_oct.cur * 12;
  return (pitch < 128) ? pitch : 255;
}

uint8_t SeqPtcPage::is_md_midi(uint8_t channel) {
  if (mcl_cfg.uart2_poly_chan - 1 == channel) {
    return POLY_EVENT;
  }
  if (mcl_cfg.uart2_ctrl_chan - 1 == channel) {
    return CTRL_EVENT;
  }
  if (mcl_cfg.md_trig_channel - 1 == channel) {
    return TRIG_EVENT;
  }

  return NO_EVENT;
  /*
    return (mcl_cfg.uart2_ctrl_chan != MIDI_LOCAL_MODE) &&
           (mcl.currentPage() != SEQ_EXTSTEP_PAGE);
  */
}
void SeqPtcMidiEvents::onNoteOnCallback_Midi2(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t channel_event = seq_ptc_page.is_md_midi(channel);

  if (channel_event) {
    if (mcl.currentPage() != SEQ_EXTSTEP_PAGE) {
      SeqPage::midi_device = midi_active_peering.get_device(UART1_PORT);
    }
  } else {
    auto active_device = midi_active_peering.get_device(UART2_PORT);
    uint8_t n = mcl_seq.find_ext_track(channel);
    if (n == 255) {
      return;
    }
    if (SeqPage::midi_device != active_device || (last_ext_track != n)) {
      SeqPage::midi_device = active_device;
      last_ext_track = min(n, NUM_EXT_TRACKS - 1);
      seq_ptc_page.config();
    } else {
      SeqPage::midi_device = active_device;
    }
  }
  uint8_t scale_padding_old = seq_ptc_page.scale_padding;
  seq_ptc_page.scale_padding = true;
  note_on(msg, channel_event);
  seq_ptc_page.scale_padding = scale_padding_old;
}

void SeqPtcMidiEvents::onNoteOffCallback_Midi2(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t channel_event = seq_ptc_page.is_md_midi(channel);
  if (channel_event) {

  } else {
    uint8_t n = mcl_seq.find_ext_track(channel);
    if (n == 255) {
      return;
    }
  }
  uint8_t scale_padding_old = seq_ptc_page.scale_padding;
  seq_ptc_page.scale_padding = true;
  note_off(msg, channel_event);
  seq_ptc_page.scale_padding = scale_padding_old;
}

void SeqPtcMidiEvents::note_on(uint8_t *msg, uint8_t channel_event) {
  uint8_t note_num = msg[1];
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  DEBUG_PRINTLN("note on");
  DEBUG_DUMP(channel);

  // pitch - MIDI_NOTE_C4
  //
  uint8_t pitch;
  bool note_on = true;

  if (channel_event) {
    if (channel_event == TRIG_EVENT) {
      if (note_num < MIDI_NOTE_C4) {
        uint8_t pos = note_num - MIDI_NOTE_C2;
        if (pos > 15) {
          return;
        }
        bool is_midi_model_ = ((MD.kit.models[pos] & 0xF0) == MID_01_MODEL);;
        if (is_midi_model_) {
          mcl_seq.md_tracks[pos].send_notes_on();
        }
        else {
          MD.triggerTrack(pos, msg[2]);
        }
        if ((seq_ptc_page.recording) && (MidiClock.state == 2)) {
          reset_undo();
          mcl_seq.md_tracks[pos].record_track(msg[2]);
        }
      }
    }
    uint8_t note = note_num - (note_num / 12) * 12;
    note_num = ((note_num / 12) - (MIDI_NOTE_C4 / 12)) * 12 + note;

    pitch = seq_ptc_page.process_ext_event(note_num, note_on, channel);
    uint8_t n = seq_ptc_page.find_arp_track(channel_event);
    arp_page.track_update(n);

    seq_ptc_page.render_arp(false, SeqPage::midi_device, n);

    if (pitch == 255)
      return;
    ArpSeqTrack *arp_track = &mcl_seq.md_arp_tracks[n];

    if (!arp_track->enabled || (MidiClock.state != 2)) {
      seq_ptc_page.trig_md(pitch, n, channel_event);
      if (mcl.currentPage() == SEQ_STEP_PAGE && channel_event == CTRL_EVENT) {
        seq_step_page.pitch_param = pitch;
      }

    }

    return;
  }
#ifdef EXT_TRACKS
  // otherwise, translate the message and send it back to MIDI2.
  pitch = seq_ptc_page.process_ext_event(note_num, note_on, channel);
  seq_ptc_page.config_encoders();

  ArpSeqTrack *arp_track = &mcl_seq.ext_arp_tracks[last_ext_track];

  arp_page.track_update();
  seq_ptc_page.render_arp(false, SeqPage::midi_device, last_ext_track);
  if (pitch == 255)
    return;

  seq_extstep_page.set_cur_y(pitch);

  if (!arp_track->enabled || (MidiClock.state != 2)) {
    seq_ptc_page.note_on_ext(pitch, msg[2]);
  }
#endif
  return;
}

void SeqPtcMidiEvents::note_off(uint8_t *msg, uint8_t channel_event) {
  DEBUG_PRINTLN(F("note off midi2"));

  uint8_t note_num = msg[1];
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t pitch;
  if (channel_event) {
    if (note_num < MIDI_NOTE_C4) {
      return;
    }
    uint8_t note = note_num - (note_num / 12) * 12;
    note_num = ((note_num / 12) - (MIDI_NOTE_C4 / 12)) * 12 + note;

    pitch = seq_ptc_page.process_ext_event(note_num, false, channel);
    uint8_t n = seq_ptc_page.find_arp_track(channel_event);
    seq_ptc_page.render_arp(false, SeqPage::midi_device, n);
    if (pitch == 255) { return; }
    ArpSeqTrack *arp_track = &mcl_seq.md_arp_tracks[n];
    bool is_midi_model_ = ((MD.kit.models[n] & 0xF0) == MID_01_MODEL);
    if (is_midi_model_) {
      if (!arp_track->enabled || (MidiClock.state != 2)) {
      mcl_seq.md_tracks[n].send_notes_off();
      }
    }
    return;
  }

#ifdef EXT_TRACKS
  pitch = seq_ptc_page.process_ext_event(note_num, false, channel);

  seq_ptc_page.config_encoders();
  seq_ptc_page.render_arp(false, SeqPage::midi_device, last_ext_track);
  arp_page.track_update();

  if (pitch == 255)
    return;

  ArpSeqTrack *arp_track = &mcl_seq.ext_arp_tracks[last_ext_track];
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

  bool send_uart2 = true;

  // CC_FWD
  //
  if (mcl_cfg.uart_cc_loopback) {
    MidiUart2.sendCC(channel, param, value);
    send_uart2 = false;
  }
  uint8_t channel_event = seq_ptc_page.is_md_midi(channel);
  if (channel_event) {
    // If external keyboard controlling MD param, send parameter updates
    // to all polyphonic tracks
    if ((param < 16) || (param > 39)) {
      return;
    }
    // If Midi2 forwarding data to port 1 , ignore this to prevent double
    // messages.
    //

    if (mcl_cfg.midi_forward_2 == 1) {
      return;
    }
    if (channel_event == POLY_EVENT) {
      for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {
        if (IS_BIT_SET16(mcl_cfg.poly_mask, n)) {
          MD.setTrackParam(n, param - 16, value, nullptr, true);
        }
      }
    }
    return;
  }

  uint8_t n = mcl_seq.find_ext_track(channel);
  if (n == 255) {
    return;
  }

  // Send mod wheel CC#1 or bank select CC#0
  if (send_uart2 && param < 2) {
    mcl_seq.ext_tracks[n].send_cc(param, value);
  }

  if (mcl.currentPage() == SEQ_EXTSTEP_PAGE) {
    if (SeqPage::pianoroll_mode > 0) {
      if (mcl_seq.ext_tracks[n].locks_params[SeqPage::pianoroll_mode - 1] - 1 ==
          PARAM_LEARN) {
        mcl_seq.ext_tracks[n].locks_params[SeqPage::pianoroll_mode - 1] =
            param + 1;
        SeqPage::param_select = param;
      }
      if (mcl_seq.ext_tracks[n].locks_params[SeqPage::pianoroll_mode - 1] - 1 ==
          param) {
        seq_extstep_page.lock_cur_y = value;
      }
    }
    if (last_ext_track == n) {
      auto &active_track = mcl_seq.ext_tracks[n];
      uint8_t timing_mid = active_track.get_timing_mid();
      int a = 16 * timing_mid;
      for (uint8_t i = 0; i < 16; i++) {

        if (note_interface.is_note_on(i)) {
          auto &active_track = mcl_seq.ext_tracks[n];

          uint8_t step = ((seq_extstep_page.cur_x / a) * 16) + i;

          active_track.clear_track_locks(step, param, 255);
          active_track.set_track_locks(step, timing_mid, param, value,
                                       SeqPage::slide);
          if (SeqPage::pianoroll_mode == 0) {
            char str[] = "CC:";
            char str2[] = "--  ";
            mcl_gui.put_value_at(value, str2);
            oled_display.textbox(str, str2);
          } else {
            uint8_t lock_idx = active_track.find_lock_idx(param);
            if (lock_idx != 255) {
              SeqPage::pianoroll_mode = lock_idx + 1;
            }
          }
        }
      }
    }
  }

  if (SeqPage::recording && (MidiClock.state == 2) &&
      !note_interface.notes_on) {
    if (param != midi_active_peering.get_device(UART2_PORT)->get_mute_cc()) {
      mcl_seq.ext_tracks[n].record_track_locks(param, value, SeqPage::slide);
    }
  }
  mcl_seq.ext_tracks[n].update_param(param, value);
}

void SeqPtcMidiEvents::onPitchWheelCallback_Midi2(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  if (seq_ptc_page.is_md_midi(channel)) {
    return;
  }

  uint8_t n = mcl_seq.find_ext_track(channel);
  if (n == 255) {
    return;
  }
  int16_t pitch = msg[1] | (msg[2] << 7);
  mcl_seq.ext_tracks[n].pitch_bend(pitch);
  if (SeqPage::recording && (MidiClock.state == 2)) {
    mcl_seq.ext_tracks[n].record_track_locks(PARAM_PB, msg[2], SeqPage::slide);
  }
}

void SeqPtcMidiEvents::onChannelPressureCallback_Midi2(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  if (seq_ptc_page.is_md_midi(channel)) {
    return;
  }
  uint8_t n = mcl_seq.find_ext_track(channel);
  if (n == 255) {
    return;
  }
  mcl_seq.ext_tracks[n].channel_pressure(msg[1]);
  if (SeqPage::recording && (MidiClock.state == 2)) {
    mcl_seq.ext_tracks[n].record_track_locks(PARAM_CHP, msg[1], false);
  }
}

void SeqPtcMidiEvents::onAfterTouchCallback_Midi2(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  if (seq_ptc_page.is_md_midi(channel)) {
    return;
  }
  uint8_t n = mcl_seq.find_ext_track(channel);
  if (n == 255) {
    return;
  }
  mcl_seq.ext_tracks[n].after_touch(msg[1], msg[2]);
}

void SeqPtcMidiEvents::onControlChangeCallback_Midi(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
  uint8_t track;
  uint8_t track_param;
  uint8_t display_polylink = 0;

  if (!seq_ptc_page.cc_link_enable) {
    return;
  }

  MD.parseCC(channel, param, &track, &track_param);
  if (track > 15) {
    return;
  }
  uint8_t start_track;
  if (track_param == 32) {
    return;
  } // don't process mute
  if (mcl_cfg.poly_mask && IS_BIT_SET16(mcl_cfg.poly_mask, track)) {

    for (uint8_t n = 0; n < 16; n++) {

      if (IS_BIT_SET16(mcl_cfg.poly_mask, n) && (n != track)) {
        if ((track_param < 24 && track_param > 7) ||
            (track_param < 8 && MD.kit.models[n] == MD.kit.models[track])) {
          MD.setTrackParam(n, track_param, value, nullptr, true);
          display_polylink = 1;
          if (mcl.currentPage() == MIXER_PAGE) { SET_BIT16(mixer_page.redraw_mask, n); }
        }
      }
      // in_sysex = 0;
    }
  }

  if (display_polylink && mcl.currentPage() != MIXER_PAGE) {
    oled_display.textbox("POLY-", "LINK");
  }
}

void SeqPtcMidiEvents::setup_midi(MidiClass *midi) {
  midi->addOnNoteOnCallback(
      this, (midi_callback_ptr_t)&SeqPtcMidiEvents::onNoteOnCallback_Midi2);
  midi->addOnNoteOffCallback(
      this, (midi_callback_ptr_t)&SeqPtcMidiEvents::onNoteOffCallback_Midi2);
  midi->addOnPitchWheelCallback(
      this, (midi_callback_ptr_t)&SeqPtcMidiEvents::onPitchWheelCallback_Midi2);
  midi->addOnAfterTouchCallback(
      this, (midi_callback_ptr_t)&SeqPtcMidiEvents::onAfterTouchCallback_Midi2);
  midi->addOnChannelPressureCallback(
      this,
      (midi_callback_ptr_t)&SeqPtcMidiEvents::onChannelPressureCallback_Midi2);
  midi->addOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&SeqPtcMidiEvents::onControlChangeCallback_Midi2);
}

void SeqPtcMidiEvents::cleanup_midi(MidiClass *midi) {
  midi->removeOnNoteOnCallback(
      this, (midi_callback_ptr_t)&SeqPtcMidiEvents::onNoteOnCallback_Midi2);
  midi->removeOnPitchWheelCallback(
      this, (midi_callback_ptr_t)&SeqPtcMidiEvents::onPitchWheelCallback_Midi2);
  midi->removeOnAfterTouchCallback(
      this, (midi_callback_ptr_t)&SeqPtcMidiEvents::onAfterTouchCallback_Midi2);
  midi->removeOnChannelPressureCallback(
      this,
      (midi_callback_ptr_t)&SeqPtcMidiEvents::onChannelPressureCallback_Midi2);
  midi->removeOnNoteOffCallback(
      this, (midi_callback_ptr_t)&SeqPtcMidiEvents::onNoteOffCallback_Midi2);
  midi->removeOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&SeqPtcMidiEvents::onControlChangeCallback_Midi);
  midi->removeOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&SeqPtcMidiEvents::onControlChangeCallback_Midi2);
}

void SeqPtcMidiEvents::setup_callbacks() {
  if (state) {
    return;
  }
  if (mcl_cfg.midi_ctrl_port == 1 || mcl_cfg.midi_ctrl_port == 3) {
    setup_midi(&Midi2);
  }
  if (mcl_cfg.midi_ctrl_port == 2 || mcl_cfg.midi_ctrl_port == 3) {
    setup_midi(&MidiUSB);
  }
  Midi.addOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&SeqPtcMidiEvents::onControlChangeCallback_Midi);
  state = true;
}

void SeqPtcMidiEvents::remove_callbacks() {
  if (!state) {
    return;
  }
  cleanup_midi(&Midi2);
  cleanup_midi(&MidiUSB);
  Midi.removeOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&SeqPtcMidiEvents::onControlChangeCallback_Midi);
  state = false;
}
