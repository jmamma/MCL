#include "MCL_impl.h"

#define FADER_LEN 18
#define FADE_RATE 0.0625

void MixerPage::set_display_mode(uint8_t param) {
  if (display_mode != param) {
    redraw_mask = -1;
    display_mode = param;
  }
}

void MixerPage::oled_draw_mutes() {
  bool is_md_device = (midi_device == &MD);

  uint8_t len = is_md_device ? mcl_seq.num_md_tracks : mcl_seq.num_ext_tracks;
  uint8_t fader_x = 0;

  for (uint8_t i = 0; i < len; ++i) {
    // draw routing
    SeqTrack *seq_track = is_md_device ? (SeqTrack*) &mcl_seq.md_tracks[i] : (SeqTrack*) &mcl_seq.ext_tracks[i];

    if (note_interface.is_note(i)) {
      oled_display.fillRect(fader_x, 2, 6, 6, WHITE);
    }

    else if (seq_track->mute_state == SEQ_MUTE_OFF) {
      oled_display.fillRect(fader_x, 2, 6, 6, BLACK);
      oled_display.drawRect(fader_x, 2, 6, 6, WHITE);
    }
    else {
      oled_display.fillRect(fader_x, 2, 6, 6, BLACK);
      oled_display.drawLine(fader_x, 5, 5 + (i * 8), 5, WHITE);
    }
    fader_x += 8;
  }
}

void MixerPage::setup() {
  encoders[0]->handler = encoder_level_handle;
  encoders[1]->handler = encoder_filtf_handle;
  encoders[2]->handler = encoder_filtw_handle;
  encoders[3]->handler = encoder_filtq_handle;
}

void MixerPage::init() {
  level_pressmode = 0;
  for (uint8_t i = 0; i < 4; i++) {
    encoders[i]->cur = 64;
    encoders[i]->old = 64;
  }
  trig_interface.on();
  bool switch_tracks = false;
  midi_events.setup_callbacks();
  oled_display.clearDisplay();
  MD.set_trigleds(0, TRIGLED_OVERLAY);
  oled_draw_mutes();
  set_display_mode(MODEL_LEVEL);
  first_track = 255;
  redraw_mask = -1;
  show_mixer_menu = 0;
}

void MixerPage::cleanup() {
  //  md_exploit.off();
  oled_display.clearDisplay();
  trig_interface.off();
  midi_events.remove_callbacks();
}

void MixerPage::set_level(int curtrack, int value) {
  // in_sysex = 1;
  MD.setTrackParam(curtrack, 33, value, nullptr, true);
  // in_sysex = 0;
}

void MixerPage::loop() {}

void MixerPage::draw_levels() {}

void encoder_level_handle(EncoderParent *enc) {

  if (mixer_page.midi_device != &MD) {
    return;
  }

  mixer_page.set_display_mode(MODEL_LEVEL);

  int dir = enc->getValue() - enc->old;
  int track_newval;

  for (int i = 0; i < 16; i++) {
    if (note_interface.is_note_on(i)) {
      track_newval = MD.kit.levels[i] + dir;
      if (track_newval < 0) {
        track_newval = 0;
      }
      if (track_newval > 127) {
        track_newval = 127;
      }
      for (uint8_t level = MD.kit.levels[i]; level < track_newval; level++) {
        mixer_page.set_level(i, level);
      }
      for (uint8_t level = MD.kit.levels[i]; level > track_newval; level--) {
        mixer_page.set_level(i, level);
      }
      // if ((MD.kit.levels[i] < 127) && (MD.kit.levels[i] > 0)) {
      mixer_page.set_level(i, track_newval);
      SET_BIT16(mixer_page.redraw_mask, i);
    }
  }
  enc->cur = 64 + dir;
  enc->old = 64;
}

void encoder_filtf_handle(EncoderParent *enc) {
  mixer_page.adjust_param(enc, MODEL_FLTF);
}

void encoder_filtw_handle(EncoderParent *enc) {
  mixer_page.adjust_param(enc, MODEL_FLTW);
}

void encoder_filtq_handle(EncoderParent *enc) {
  mixer_page.adjust_param(enc, MODEL_FLTQ);
}

void encoder_lastparam_handle(EncoderParent *enc) {
  mixer_page.adjust_param(enc, MD.midi_events.last_md_param);
}

void MixerPage::adjust_param(EncoderParent *enc, uint8_t param) {

  if (midi_device != &MD) {
    return;
  }

  set_display_mode(param);

  int dir = enc->getValue() - enc->old;
  int newval;

  for (int i = 0; i < 16; i++) {
    if (note_interface.is_note_on(i)) {
      newval = MD.kit.params[i][param] + dir;
      if (newval < 0) {
        newval = 0;
      }
      if (newval > 127) {
        newval = 127;
      }
      for (uint8_t value = MD.kit.params[i][param]; value < newval; value++) {
        MD.setTrackParam(i, param, value, nullptr, true);
      }
      for (uint8_t value = MD.kit.params[i][param]; value > newval; value--) {
        MD.setTrackParam(i, param, value, nullptr, true);
      }
      MD.setTrackParam(i, param, newval, nullptr, true);
      SET_BIT16(redraw_mask, i);
    }
  }
  enc->cur = 64 + dir;
  enc->old = 64;
}

void MixerPage::display() {

  auto oldfont = oled_display.getFont();

  uint8_t fader_level;
  uint8_t meter_level;
  uint8_t fader_x = 0;
  constexpr uint8_t fader_y = 11;

  if (oled_display.textbox_enabled) {
    oled_display.clearDisplay();
    oled_draw_mutes();
    redraw_mask = -1;
  }
  bool is_md_device = (midi_device == &MD);

  uint8_t len = is_md_device ? mcl_seq.num_md_tracks : mcl_seq.num_ext_tracks;
  uint8_t *levels = is_md_device ? disp_levels : ext_disp_levels;

  if (seq_step_page.display_mute_mask(midi_device)) {
    oled_draw_mutes();
  }

  uint8_t dec = MidiClock.get_tempo() * FADE_RATE;

  for (int i = 0; i < len; i++) {

    if (is_md_device) {
      if (display_mode == MODEL_LEVEL) {
        fader_level = MD.kit.levels[i];
      } else {
        fader_level = MD.kit.params[i][display_mode];
      }
    } else {
      fader_level = 127;
    }

    fader_level = ((fader_level * 0.00787) * FADER_LEN) + 0;
    meter_level = ((levels[i] * 0.00787) * FADER_LEN) + 0;
    meter_level = min(fader_level, meter_level);

    if (IS_BIT_SET16(redraw_mask, i)) {
      oled_display.fillRect(fader_x, fader_y - 1, 6, FADER_LEN + 1, BLACK);
      oled_display.drawRect(fader_x, fader_y + (FADER_LEN - fader_level), 6,
                            fader_level + 2, WHITE);
    }
    if (note_interface.is_note_on(i)) {
      oled_display.fillRect(fader_x, fader_y + 1 + (FADER_LEN - fader_level), 6,
                            fader_level, WHITE);
    } else {

      oled_display.fillRect(fader_x + 1,
                            fader_y + 1 + (FADER_LEN - fader_level), 4,
                            FADER_LEN - meter_level - 1, BLACK);
      oled_display.fillRect(fader_x + 1,
                            fader_y + 1 + (FADER_LEN - meter_level), 4,
                            meter_level + 1, WHITE);
    }
    fader_x += 8;

    CLEAR_BIT16(redraw_mask, i);

    if (levels[i] < dec) {
      levels[i] = 0;
    } else {
      levels[i] -= dec;
    }
  }

  if (!redraw_mask) {
    oled_display.display();
  } else {
    redraw_mask = -1;
  }
  oled_display.setFont(oldfont);
}

void MixerPage::disable_record_mutes() {

  MidiDevice *devs[2] = {
      midi_active_peering.get_device(UART1_PORT),
      midi_active_peering.get_device(UART2_PORT),
  };

  ElektronDevice *elektron_devs[2] = {
      devs[0]->asElektronDevice(),
      devs[1]->asElektronDevice(),
  };

  for (uint8_t n = 0; n < mcl_seq.num_md_tracks; n++) {
    if (n < mcl_seq.num_ext_tracks) {
      if (mcl_seq.ext_tracks[n].record_mutes) {
        mcl_seq.ext_tracks[n].record_mutes = false;
        if (mcl_seq.ext_tracks[n].mute_state == SEQ_MUTE_ON) { mcl_seq.ext_tracks[n].toggle_mute(); devs[1]->muteTrack(n, SEQ_MUTE_OFF); }
      }
    }
    if (mcl_seq.md_tracks[n].record_mutes) {
      mcl_seq.md_tracks[n].record_mutes = false;
      if (mcl_seq.md_tracks[n].mute_state == SEQ_MUTE_ON) { mcl_seq.md_tracks[n].toggle_mute(); devs[0]->muteTrack(n, SEQ_MUTE_OFF); }
    }
  }
  if (!seq_step_page.recording) {
    clearLed2();
  }
}

bool MixerPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    uint8_t device = midi_active_peering.get_device(port)->id;

    uint8_t track = event->source - 128;

    bool is_md_device = (midi_device == &MD);

    uint8_t len = is_md_device ? mcl_seq.num_md_tracks : mcl_seq.num_ext_tracks;

    if (track > 16) {
      return false;
    }

    if (!show_mixer_menu) {
      trig_interface.send_md_leds(TRIGLED_OVERLAY);
    }

    if (event->mask == EVENT_BUTTON_PRESSED && track <= len) {
      if (note_interface.is_note(track)) {
        if (show_mixer_menu) {

          SeqTrack *seq_track = is_md_device ? (SeqTrack*) &mcl_seq.md_tracks[track] : (SeqTrack*) &mcl_seq.ext_tracks[track];

          if (is_md_device) { seq_track->toggle_mute(); }
          else { mcl_seq.ext_tracks[track].toggle_mute(); }

          midi_device->muteTrack(track, seq_track->mute_state);

          if (trig_interface.is_key_down(MDX_KEY_PATSONGKIT)) {
            seq_track->record_mutes = true;
          }
          oled_draw_mutes();
        } else if (first_track == 255) {
          first_track = track;
          MD.setStatus(0x22, track);
        }
      }
      return true;
    }

    if (event->mask == EVENT_BUTTON_RELEASED) {
      SET_BIT16(redraw_mask, track);
      if (note_interface.notes_count_on() == 0) {
        first_track = 255;
        note_interface.init_notes();
        oled_draw_mutes();
      }
      return true;
    }
  }
  /*
    if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
            route_page.toggle_routes_batch();
          note_interface.init_notes();
  #ifdef OLED_DISPLAY
         route_page.draw_routes(0);
  #endif
    }
  */

  if (EVENT_CMD(event)) {
    uint8_t key = event->source - 64;
    if (event->mask == EVENT_BUTTON_PRESSED) {
      switch (key) {
      case MDX_KEY_PATSONGKIT: {
        setLed2();
        break;
      }
      case MDX_KEY_NO: {
        if (note_interface.notes_count_on() == 0) {
          disable_record_mutes();
          GUI.setPage(&grid_page);
          return true;
        }
        if (midi_device == &MD) {
          for (uint8_t i = 0; i < 16; i++) {
            if (note_interface.is_note_on(i)) {
              for (uint8_t c = 0; c < 24; c++) {
                MD.restore_kit_param(i, c);
              }
              mcl_seq.md_tracks[i].update_params();
            }
          }
        }
        break;
      }
      case MDX_KEY_YES: {
        if (midi_device != &MD) {
          midi_device = &MD;
        } else {
          midi_device = midi_active_peering.get_device(UART2_PORT);
        }
        redraw_mask = -1;
        oled_display.clearDisplay();
        oled_draw_mutes();
        break;
      }
      }
    }
    if (event->mask == EVENT_BUTTON_RELEASED) {
      switch (key) {
      case MDX_KEY_PATSONGKIT: {
        disable_record_mutes();
        MD.set_trigleds(0, TRIGLED_OVERLAY);
        break;
      }
      }
    }
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON3) && !BUTTON_DOWN(Buttons.BUTTON4)) {
    seq_step_page.mute_mask = 0;
    show_mixer_menu = true;
    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON3)) {
    show_mixer_menu = false;
    disable_record_mutes();
    MD.set_trigleds(0, TRIGLED_EXCLUSIVE);
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    trig_interface.on();
    disable_record_mutes();
    GUI.setPage(&page_select_page);
    return true;
  }

  /*
  if (EVENT_PRESSED(event, Buttons.ENCODER1) ||
      EVENT_PRESSED(event, Buttons.ENCODER2) ||
      EVENT_PRESSED(event, Buttons.ENCODER3) ||
      EVENT_PRESSED(event, Buttons.ENCODER4)) {
    //    if (note_interface.notes_count() == 0) {
    //      route_page.update_globals();
    //      GUI.setPage(&grid_page);
    //    }
    return true;
  }
  */
  return false;
}

void MixerMidiEvents::setup_callbacks() {
  if (state) {
    return;
  }
  Midi.addOnNoteOnCallback(
      this, (midi_callback_ptr_t)&MixerMidiEvents::onNoteOnCallback_Midi);
  Midi.addOnNoteOffCallback(
      this, (midi_callback_ptr_t)&MixerMidiEvents::onNoteOffCallback_Midi);
  Midi.addOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&MixerMidiEvents::onControlChangeCallback_Midi);

  state = true;
}

void MixerMidiEvents::remove_callbacks() {
  if (!state) {
    return;
  }

  DEBUG_PRINTLN(F("remove calblacks"));
  Midi.removeOnNoteOnCallback(
      this, (midi_callback_ptr_t)&MixerMidiEvents::onNoteOnCallback_Midi);
  Midi.removeOnNoteOffCallback(
      this, (midi_callback_ptr_t)&MixerMidiEvents::onNoteOffCallback_Midi);
  Midi.removeOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&MixerMidiEvents::onControlChangeCallback_Midi);

  state = false;
}

void MixerMidiEvents::onControlChangeCallback_Midi(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
  uint8_t track;
  uint8_t track_param;

  MD.parseCC(channel, param, &track, &track_param);
  if (track > 15) {
    return;
  }
  if (track_param == 32) {
    return;
  } // don't process mute
  if (mixer_page.midi_device != &MD) { return; }
  SET_BIT16(mixer_page.redraw_mask, track);
  for (int i = 0; i < 16; i++) {
    if (note_interface.is_note_on(i) && (i != track)) {
      MD.setTrackParam(i, track_param, value, nullptr, true);
      if (track_param < 24) {
        mcl_seq.md_tracks[i].update_param(track_param, value);
      }
      SET_BIT16(mixer_page.redraw_mask, i);
    }
  }
  mixer_page.set_display_mode(track_param);
}

uint8_t MixerMidiEvents::note_to_trig(uint8_t note_num) {
  uint8_t trig_num = 0;
  for (uint8_t i = 0; i < sizeof(MD.global.drumMapping); i++) {
    if (note_num == MD.global.drumMapping[i]) {
      trig_num = i;
    }
  }
  return trig_num;
}
void MixerMidiEvents::onNoteOnCallback_Midi(uint8_t *msg) {
  uint8_t note_num = msg[1];
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);

  uint8_t n = note_to_trig(msg[1]);
  if (msg[0] != 153) {
    mixer_page.disp_levels[n] = MD.kit.levels[n];
  }
}
void MixerMidiEvents::onNoteOffCallback_Midi(uint8_t *msg) {}
