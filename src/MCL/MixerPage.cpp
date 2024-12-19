#include "MixerPage.h"
#include "AuxPages.h"
#include "ResourceManager.h"
#include "MCLGUI.h"
#include "MCLSeq.h"
#include "SeqPages.h"

#define FADER_LEN 18
#define FADE_RATE 8

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

  bool draw = true;
  if (preview_mute_set != 255 && load_types[preview_mute_set][!is_md_device] == 0) { draw = false; }
  for (uint8_t i = 0; i < len; ++i) {
    // draw routing
    SeqTrack *seq_track = is_md_device ? (SeqTrack *)&mcl_seq.md_tracks[i]
                                       : (SeqTrack *)&mcl_seq.ext_tracks[i];

    uint8_t mute_state =
        preview_mute_set != 255
            ? IS_BIT_SET16(mute_sets[!is_md_device].mutes[preview_mute_set], i)
            : seq_track->mute_state == SEQ_MUTE_OFF;

    //   if (note_interface.is_note(i)) {
    //   oled_display.fillRect(fader_x, 2, 6, 6, WHITE);
    // } else if (mute_state) {
    // No Mute (SEQ_MUTE_OFF)
    oled_display.fillRect(fader_x, 2, 6, 6, BLACK);
    if (draw) {
      if (mute_state) {
        oled_display.drawRect(fader_x, 2, 6, 6, WHITE);
      } else {
        oled_display.drawLine(fader_x, 5, 5 + (i * 8), 5, WHITE);
      }
    }
    fader_x += 8;
  }
}

void MixerPage::setup() {
  /*
encoders[0]->handler = encoder_level_handle;
encoders[1]->handler = encoder_filtf_handle;
encoders[2]->handler = encoder_filtw_handle;
encoders[3]->handler = encoder_filtq_handle;
*/
}

void MixerPage::init() {
  DEBUG_PRINTLN("mixer init");
  level_pressmode = 0;
  /*
  for (uint8_t i = 0; i < 4; i++) {
    encoders[i]->cur = 64;
    encoders[i]->old = 64;
  }
  */
  MD.set_key_repeat(0);
  trig_interface.on();
  bool is_md_device = (midi_device == &MD);
  MD.set_trigleds(0, is_md_device ? TRIGLED_OVERLAY : TRIGLED_EXCLUSIVE);
  preview_mute_set = 255;
  bool switch_tracks = false;
  oled_display.clearDisplay();
  set_display_mode(MODEL_LEVEL);
  first_track = 255;
  redraw_mask = -1;
  seq_step_page.mute_mask++;
  show_mixer_menu = 0;
  // populate_mute_set();
  draw_encoders = false;
  redraw_mutes = true;
  R.Clear();
  R.use_icons_knob();
  //  R.use_machine_param_names();
}

void MixerPage::cleanup() {
  //  md_exploit.off();
  MD.set_key_repeat(1);
  disable_record_mutes();
  trig_interface.off();
  ext_key_down = 0;
  mute_toggle = 0;
}

void MixerPage::set_level(int curtrack, int value) {
  // in_sysex = 1;
  MD.setTrackParam(curtrack, 33, value, nullptr, true);
  // in_sysex = 0;
}

void MixerPage::load_perf_locks(uint8_t state) {
  for (uint8_t n = 0; n < GUI_NUM_ENCODERS; n++) {
    PerfEncoder *enc = (PerfEncoder*) encoders[n];
    uint8_t val = perf_locks[state][n];
    if (val < 128) {
      enc->cur = val;
      enc->resend = true;
    }
  }
}
void MixerPage::loop() {
  constexpr int timeout = 750;
  perf_page.func_enc_check();
  bool old_draw_encoders = draw_encoders;

  if ((trig_interface.is_key_down(MDX_KEY_NO))&& preview_mute_set != 255 &&
      note_interface.notes_on == 0) {
    for (uint8_t n = 0; n < GUI_NUM_ENCODERS; n++) {
      PerfEncoder *enc = (PerfEncoder*) encoders[n];
      if (enc->hasChanged()) {
        if (BUTTON_DOWN(Buttons.ENCODER1 + n)) {
          GUI.ignoreNextEvent(Buttons.ENCODER1 + n);
        }
        perf_locks[preview_mute_set][n] = enc->cur;
        enc->old = enc->cur;
      }
    }
  }

  perf_page.encoder_send();

  if (draw_encoders && trig_interface.is_key_down(MDX_KEY_FUNC)) {
    draw_encoders = true;
  } else {
    draw_encoders = false;
      uint64_t mask =
          ((uint64_t)1 << MDX_KEY_LEFT) | ((uint64_t)1 << MDX_KEY_UP) |
          ((uint64_t)1 << MDX_KEY_RIGHT) | ((uint64_t)1 << MDX_KEY_DOWN) |
          ((uint64_t)1 << MDX_KEY_YES);
    for (uint8_t n = 0; n < 4; n++) {
     bool check = (trig_interface.cmd_key_state & mask);

      if (note_interface.notes_on || check) {
        encoders_used_clock[n] = g_clock_ms + timeout + 1;
      }
      if (mcl_gui.show_encoder_value(encoders[n], timeout)) {
        draw_encoders = true;
      }
    }
  }
  if (draw_encoders != old_draw_encoders) {
    if (!draw_encoders) {
      redraw();
    }
  }

  if (!draw_encoders) {
    init_encoders_used_clock(timeout);
  }
}
void MixerPage::draw_levels() {}

void encoder_level_handle(EncoderParent *enc) {

  if (mixer_page.midi_device != &MD) {
    return;
  }

  mixer_page.set_display_mode(MODEL_LEVEL);

  int dir = enc->getValue() - enc->old;
  int track_newval;

  for (uint8_t i = 0; i < 16; i++) {
    if (note_interface.is_note_on(i)) {
      track_newval = min(max(MD.kit.levels[i] + dir, 0), 127);
      mixer_page.set_level(i, track_newval);
      SET_BIT16(mixer_page.redraw_mask, i);
    }
  }
  enc->cur = 64 + dir;
  enc->old = 64;
}

void send_fx(uint8_t param, EncoderParent *enc, uint8_t type) {
  //  for (int val = enc->old; val > enc->cur; val--) {
  MD.setFXParam(param, enc->cur, type, true);
  //  }
  //  for (int val = enc->old; val > enc->cur; val++) {
  //  MD.sendFXParam(param, val, type);
  //  }
  PGM_P param_name = NULL;
  char str[4];
  char str2[] = "--  ";

  param_name = fx_param_name(type, param);
  strncpy(str, param_name, 4);
  mcl_gui.put_value_at(enc->cur, str2);
  oled_display.textbox(str, str2);
}

/*
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
*/

void MixerPage::draw_encs() {
  constexpr uint8_t fader_y = 11;
  oled_display.fillRect(0, fader_y, 128, 21, BLACK);
  for (uint8_t n = 0; n < 4; n++) {
    char str1[] = "A";
    str1[0] = 'A' + n;
    uint8_t pos = n * 24;
    bool highlight =
          (preview_mute_set != 255) && (perf_locks[preview_mute_set][n] != 255); // && (trig_interface.is_key_down(MDX_KEY_NO));
    uint8_t val =
          highlight ? perf_locks[preview_mute_set][n] : encoders[n]->cur;
    mcl_gui.draw_encoder(24 + pos, fader_y + 4, val, highlight);
    oled_display.setCursor(16 + pos, fader_y + 6);
    oled_display.print(str1);
  }
}

void MixerPage::adjust_param(EncoderParent *enc, uint8_t param) {

  if (midi_device != &MD) {
    return;
  }

  set_display_mode(param);

  int dir = enc->getValue() - enc->old;
  int newval;

  for (uint8_t i = 0; i < 16; i++) {
    if (note_interface.is_note_on(i)) {
      newval = min(max(MD.kit.params[i][param] + dir, 0), 127);
      MD.setTrackParam(i, param, newval, nullptr, true);
      SET_BIT16(redraw_mask, i);
    }
  }
  enc->cur = 64 + dir;
  enc->old = 64;
}

void MixerPage::display() {

  if (oled_display.textbox_enabled) {
    redraw();
  }

  if (redraw_mutes) {
    oled_draw_mutes();
    redraw_mutes = false;
  }

  bool is_md_device = (midi_device == &MD);
  constexpr uint8_t fader_y = 11;

  uint8_t mute_set = preview_mute_set;

  if (((ext_key_down && mute_set == 255) || show_mixer_menu) && seq_step_page.display_mute_mask(midi_device)) {
    oled_draw_mutes();
  } else if (mute_set != 255 && mute_sets[!is_md_device].mutes[mute_set] !=
                                    seq_step_page.mute_mask) {

    uint16_t mask = mute_sets[!is_md_device].mutes[mute_set];
    if (load_types[mute_set][!is_md_device] == 0) { mask = 0; }
    if (!is_md_device) {
      mask &= 0b111111;
    }
    MD.set_trigleds(mask, is_md_device ? TRIGLED_OVERLAY : TRIGLED_EXCLUSIVE);
    seq_step_page.mute_mask = mask;
    oled_draw_mutes();
  }
  if (draw_encoders || preview_mute_set != 255) {
    // oled_display.clearDisplay();
    draw_encs();
    oled_display.setFont(&TomThumb);
    if (load_mute_set != 255 && load_mute_set == preview_mute_set) {
      oled_display.setCursor(111, 31);
      oled_display.print("LOAD");
    }
    oled_display.setFont();
  } else {

    uint8_t fader_level;
    uint8_t meter_level;
    uint8_t fader_x = 0;

    uint8_t len = is_md_device ? mcl_seq.num_md_tracks : mcl_seq.num_ext_tracks;
    uint8_t *levels = is_md_device ? disp_levels : ext_disp_levels;

    uint8_t dec = FADE_RATE;

    for (uint8_t i = 0; i < len; i++) {

      if (is_md_device) {
        if (display_mode == MODEL_LEVEL) {
          fader_level = MD.kit.levels[i];
        } else {
          fader_level = MD.kit.params[i][display_mode];
        }
      } else {
        fader_level = 127;
      }

      fader_level = (((uint16_t) fader_level * FADER_LEN) / 127) + 0;
      meter_level = (((uint16_t) levels[i] * FADER_LEN) / 127) + 0;
      meter_level = min(fader_level, meter_level);

      if (IS_BIT_SET16(redraw_mask, i)) {
        oled_display.fillRect(fader_x, fader_y - 1, 6, FADER_LEN + 1, BLACK);
        oled_display.drawRect(fader_x, fader_y + (FADER_LEN - fader_level), 6,
                              fader_level + 2, WHITE);
      }
      if (note_interface.is_note_on(i)) {
        oled_display.fillRect(fader_x, fader_y + 1 + (FADER_LEN - fader_level),
                              6, fader_level, WHITE);
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
  }

  redraw_mask = -1;
}

void MixerPage::record_mutes_set(bool state) {
  bool is_md_device = (midi_device == &MD);
  for (uint8_t i = 0; i < 16; i++) {
    if (note_interface.is_note_on(i)) {
      if (is_md_device) {
        mcl_seq.md_tracks[i].record_mutes = state;
        if (!state)
          mcl_seq.md_tracks[i].clear_mute();
      } else if (i < mcl_seq.num_ext_tracks) {
        mcl_seq.ext_tracks[i].record_mutes = state;
        if (!state)
          mcl_seq.ext_tracks[i].clear_mute();
      }
    }
  }
}

void MixerPage::disable_record_mutes(bool clear) {
  for (uint8_t n = 0; n < mcl_seq.num_md_tracks; n++) {
    if (n < mcl_seq.num_ext_tracks) {
      mcl_seq.ext_tracks[n].record_mutes = false;
      if (clear) {
        mcl_seq.ext_tracks[n].clear_mute();
      }
    }
    mcl_seq.md_tracks[n].record_mutes = false;
    if (clear) {
      mcl_seq.md_tracks[n].clear_mute();
    }
  }
  if (!seq_step_page.recording) {
    clearLed2();
  }
}

void MixerPage::populate_mute_set() {
  if (current_mute_set == 255) {
    return;
  }
  for (uint8_t dev = 0; dev < 2; dev++) {

    uint8_t len = (dev == 0) ? mcl_seq.num_md_tracks : mcl_seq.num_ext_tracks;

    for (uint8_t n = 0; n < len; n++) {
      SeqTrack *seq_track = (dev == 0) ? (SeqTrack *)&mcl_seq.md_tracks[n]
                                       : (SeqTrack *)&mcl_seq.ext_tracks[n];
      if (seq_track->mute_state == SEQ_MUTE_ON) {
        CLEAR_BIT16(mute_sets[dev].mutes[current_mute_set], n);
      } else {
        SET_BIT16(mute_sets[dev].mutes[current_mute_set], n);
      }
    }
  }
}

void MixerPage::switch_mute_set(uint8_t state, bool load_perf, bool *load_type) {

  MidiDevice *devs[2] = {
      midi_active_peering.get_device(UART1_PORT),
      midi_active_peering.get_device(UART2_PORT),
  };
  if (load_type != nullptr && state < 255) {
    for (uint8_t dev = 0; dev < 2; dev++) {
      bool is_md_device = dev == 0;

      if (!load_type[dev]) continue;
      uint8_t len =
        (is_md_device) ? mcl_seq.num_md_tracks : mcl_seq.num_ext_tracks;

      for (uint8_t n = 0; n < len; n++) {
        SeqTrack *seq_track = (is_md_device) ? (SeqTrack *)&mcl_seq.md_tracks[n]
                                           : (SeqTrack *)&mcl_seq.ext_tracks[n];

        bool mute_state = IS_BIT_CLEAR16(mute_sets[dev].mutes[state], n);
        // Flip
        if (state == 4 && devs[dev] == midi_device) {
          mute_state = !seq_track->mute_state;
        }
        // Switch
        if (mute_state != seq_track->mute_state) {
          devs[dev]->muteTrack(n, mute_state);
          if (is_md_device) {
            mcl_seq.md_tracks[n].toggle_mute();
          } else {
            mcl_seq.ext_tracks[n].toggle_mute();
          }
        }
      }
    }
  }
  if (state < 4 && load_perf) {
    load_perf_locks(state);
  }
  redraw_mutes = true;
}

uint8_t MixerPage::get_mute_set(uint8_t key) {
  switch (key) {
  case MDX_KEY_LEFT:
    return 1;
  case MDX_KEY_UP:
    return 2;
  case MDX_KEY_RIGHT:
    return 3;
  }
  return 0;
}

void MixerPage::redraw() {
  redraw_mask = -1;
  seq_step_page.mute_mask++;
  oled_display.clearDisplay();
  oled_draw_mutes();
}

void MixerPage::toggle_or_solo(bool solo) {
  uint8_t is_md_device = (midi_device == &MD);
  uint8_t len = is_md_device ? mcl_seq.num_md_tracks : mcl_seq.num_ext_tracks;
  for (uint8_t i = 0; i < len; i++) {
    bool note_on = note_interface.is_note_on(i);
    bool mute_state = false;

    if (solo) {
      if (is_md_device) {
        if (mcl_seq.md_tracks[i].mute_state == note_on) {
          mcl_seq.md_tracks[i].toggle_mute();
          mute_state = mcl_seq.md_tracks[i].mute_state;
        }
      } else {
        if (mcl_seq.ext_tracks[i].mute_state == note_on) {
          mcl_seq.ext_tracks[i].toggle_mute();
          mute_state = mcl_seq.ext_tracks[i].mute_state;
        }
      }
      midi_device->muteTrack(i, !note_on);
    } else if (note_on) {
      // TOGGLE
      if (is_md_device) {
        mcl_seq.md_tracks[i].toggle_mute();
        mute_state = mcl_seq.md_tracks[i].mute_state;
      } else {
        mcl_seq.ext_tracks[i].toggle_mute();
        mute_state = mcl_seq.ext_tracks[i].mute_state;
      }
      midi_device->muteTrack(i, mute_state);
    }
  }
  oled_draw_mutes();
}

bool MixerPage::handleEvent(gui_event_t *event) {

  uint8_t is_md_device = (midi_device == &MD);
  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    uint8_t device = midi_active_peering.get_device(port)->id;

    uint8_t track = event->source - 128;


    if (track > 16) {
      return false;
    }
    if (!ext_key_down && !show_mixer_menu && preview_mute_set == 255) {
      trig_interface.send_md_leds(is_md_device ? TRIGLED_OVERLAY : TRIGLED_EXCLUSIVE);
    }

    uint8_t len = is_md_device ? mcl_seq.num_md_tracks : mcl_seq.num_ext_tracks;
    if (event->mask == EVENT_BUTTON_PRESSED && track < len) {
      if (note_interface.is_note(track)) {
        if (show_mixer_menu || preview_mute_set != 255 && load_types[preview_mute_set][!is_md_device] || ext_key_down) {
          if (ext_key_down) { mute_toggle = 1; }
          SeqTrack *seq_track = is_md_device
                                    ? (SeqTrack *)&mcl_seq.md_tracks[track]
                                    : (SeqTrack *)&mcl_seq.ext_tracks[track];

          uint8_t mute_set = preview_mute_set;

          // Toggle active mutes
          if (mute_set == 255) {
            if (is_md_device) {
              mcl_seq.md_tracks[track].toggle_mute();
            } else {
              mcl_seq.ext_tracks[track].toggle_mute();
            }
            midi_device->muteTrack(track, seq_track->mute_state);
            return true;
          }

          // Toggle preview mutes
          bool state =
              IS_BIT_SET16(mute_sets[!is_md_device].mutes[mute_set], track);
          if (state == SEQ_MUTE_ON) {
            CLEAR_BIT16(mute_sets[!is_md_device].mutes[mute_set], track);
          } else {
            SET_BIT16(mute_sets[!is_md_device].mutes[mute_set], track);
          }

          // oled_draw_mutes();
        } else if (first_track == 255 && midi_device == &MD) {
          first_track = track;
          MD.setStatus(0x22, track);
        }
        oled_draw_mutes();
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
      case MDX_KEY_KIT: {
        record_mutes_set(false);
        // trig_interface.ignore_next_event(GLOBAL);
        break;
      }
      case MDX_KEY_PATSONGKIT: {
        bool clear = true;
        disable_record_mutes(true);
        // trig_interface.ignore_next_event(GLOBAL);
        break;
      }
      case MDX_KEY_EXTENDED: {
        DEBUG_PRINTLN("key extended");
        ext_key_down = 1;
        redraw();
        if (midi_device == &MD) {
          for (uint8_t i = 0; i < 16; i++) {
            if (note_interface.is_note_on(i)) {
              for (uint8_t c = 0; c < 24; c++) {
                MD.restore_kit_param(i, c);
              }
            }
          }
        }
        break;
      }
      case MDX_KEY_NO: {
        uint64_t mask =
            ((uint64_t)1 << MDX_KEY_LEFT) | ((uint64_t)1 << MDX_KEY_UP) |
            ((uint64_t)1 << MDX_KEY_RIGHT) | ((uint64_t)1 << MDX_KEY_DOWN);
        if (((trig_interface.cmd_key_state & mask) == 0)) {
          if (note_interface.notes_count_on() == 0) {
            mcl.setPage(fx_page_a.last_page);
            return true;
          }
          toggle_or_solo(true);
        }
        break;
      }
      case MDX_KEY_BANKA: {
        if (preview_mute_set != 255) {
          load_types[preview_mute_set][!is_md_device] = !load_types[preview_mute_set][!is_md_device];
          if (load_types[preview_mute_set][!is_md_device] == 0) { seq_step_page.mute_mask = 0; }
          redraw_mutes = true;
          return true;
        }
        break;
      }
      case MDX_KEY_BANKB: {
        if (preview_mute_set != 255) {
          if (load_mute_set == preview_mute_set) { load_mute_set = 255; }
          else { load_mute_set = preview_mute_set; }
          return true;
        }
        break;
      }
      case MDX_KEY_YES: {
       if (preview_mute_set == 255 &&
            trig_interface.is_key_down(MDX_KEY_FUNC) &&
            note_interface.notes_on == 0) {
            bool load_t[2] = { 0, 0 };
            load_t[!is_md_device] = 1;
            switch_mute_set(4,false,load_t); //---> Flip mutes
          break;
        }
        uint8_t set = 255;

        if (trig_interface.is_key_down(MDX_KEY_LEFT)) {
          set = 1;
        } else if (trig_interface.is_key_down(MDX_KEY_UP)) {
          set = 2;
        } else if (trig_interface.is_key_down(MDX_KEY_RIGHT)) {
          set = 3;
        } else if (trig_interface.is_key_down(MDX_KEY_DOWN)) {
          set = 0;
        }
        if (set != 255) {
          //bool load_t[2];
          //load_t[0] = load_types[key][0];
          //load_t[1] = load_types[key][1];
          //load_t[is_md_device] = 0;
          switch_mute_set(set,true,load_types[set]);
        }
        else {
          if (!note_interface.notes_on) {
            seq_step_page.mute_mask = 0;
            show_mixer_menu = true;
          } else {
            toggle_or_solo();
          }
        }
        break;
      }
      case MDX_KEY_LEFT:
      case MDX_KEY_UP:
      case MDX_KEY_RIGHT:
      case MDX_KEY_DOWN: {
        if (trig_interface.is_key_down(MDX_KEY_NO)) { return true; }
        uint8_t set = get_mute_set(key);
        if (trig_interface.is_key_down(MDX_KEY_YES)) {
          switch_mute_set(set,true,load_types[set]);
        } else {
            preview_mute_set = set;
          // force redraw in display()
          seq_step_page.mute_mask++;
        }
        break;
      }
      case MDX_KEY_SCALE: {

        if (midi_device != &MD) {
          midi_device = &MD;
        } else {
          midi_device = midi_active_peering.get_device(UART2_PORT);
        }
        is_md_device = (midi_device == &MD);
        trig_interface.send_md_leds(is_md_device ? TRIGLED_OVERLAY : TRIGLED_EXCLUSIVE);
        redraw();
        break;
      }
      }
    }
    if (event->mask == EVENT_BUTTON_RELEASED) {
      switch (key) {
      case MDX_KEY_GLOBAL:
      case MDX_KEY_YES: {
        goto global_release;
      }
      case MDX_KEY_EXTENDED: {

        ext_key_down = 0;
        if (note_interface.notes_on == 0 && !mute_toggle) {
            if (last_page != NULL_PAGE) { mcl.setPage(last_page); last_page = NULL_PAGE; } 
            else { mcl.setPage(GRID_PAGE); }
            return true;
        }
        mute_toggle = 0;
        if (!show_mixer_menu && preview_mute_set == 255) {
         trig_interface.send_md_leds(is_md_device ? TRIGLED_OVERLAY : TRIGLED_EXCLUSIVE);
        }
        return true;
      }
      case MDX_KEY_LEFT:
      case MDX_KEY_UP:
      case MDX_KEY_RIGHT:
      case MDX_KEY_DOWN: {
        uint64_t mask =
            ((uint64_t)1 << MDX_KEY_LEFT) | ((uint64_t)1 << MDX_KEY_UP) |
            ((uint64_t)1 << MDX_KEY_RIGHT) | ((uint64_t)1 << MDX_KEY_DOWN) | ((uint64_t)1 << MDX_KEY_YES);
        if ((trig_interface.cmd_key_state & mask) == 0) {
          trig_interface.send_md_leds(is_md_device ? TRIGLED_OVERLAY : TRIGLED_EXCLUSIVE);
          preview_mute_set = 255;
          redraw();
        }
        break;
      }
      }
    }
  }
  if (EVENT_RELEASED(event, Buttons.BUTTON3)) {
    global_release:
      preview_mute_set = 255;
      show_mixer_menu = false;
      disable_record_mutes();
      MD.set_trigleds(0, is_md_device ? TRIGLED_OVERLAY : TRIGLED_EXCLUSIVE);
      redraw();
    return true;
  }
  if (EVENT_PRESSED(event,Buttons.BUTTON3)) {
     // show_mixer_menu = true;
        if (note_interface.notes_on) {
          setLed2();
          record_mutes_set(true);
          return true;
        }
        if (BUTTON_DOWN(Buttons.ENCODER1)) {
           perf_param1.clear_scenes();
        }
        if (BUTTON_DOWN(Buttons.ENCODER2)) {
           perf_param2.clear_scenes();
        }
        if (BUTTON_DOWN(Buttons.ENCODER3)) {
           perf_param3.clear_scenes();
        }
        if (BUTTON_DOWN(Buttons.ENCODER4)) {
           perf_param4.clear_scenes();
        }
           redraw_mask = -1;
          return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
        if (BUTTON_DOWN(Buttons.ENCODER1)) {
           perf_param1.scene_autofill();
        }
        if (BUTTON_DOWN(Buttons.ENCODER2)) {
           perf_param2.scene_autofill();
        }
        if (BUTTON_DOWN(Buttons.ENCODER3)) {
           perf_param3.scene_autofill();
        }
        if (BUTTON_DOWN(Buttons.ENCODER4)) {
           perf_param4.scene_autofill();
        }
          redraw_mask = -1;
          return true;
  }

  if (preview_mute_set != 255 && (trig_interface.is_key_down(MDX_KEY_NO))) {
    if (event->source >= Buttons.ENCODER1 &&
        event->source <= Buttons.ENCODER4) {
      uint8_t b = event->source - Buttons.ENCODER1;
      if ((event)->mask & EVENT_BUTTON_RELEASED) {
        perf_locks[preview_mute_set][b] = 255;
      }
      if ((event)->mask & EVENT_BUTTON_PRESSED) {
        if (perf_locks[preview_mute_set][b] == 255) {
          GUI.ignoreNextEvent(event->source);
          perf_locks[preview_mute_set][b] = encoders[b]->cur;
        }
      }
      return true;
    }
  }

  return false;
}

/*
void MixerMidiEvents::setup_callbacks() {
  if (state) {
    return;
  }
  Midi.addOnNoteOnCallback(
      this, (midi_callback_ptr_t)&MixerMidiEvents::onNoteOnCallback_Midi);
  //Midi.addOnNoteOffCallback(
  //    this, (midi_callback_ptr_t)&MixerMidiEvents::onNoteOffCallback_Midi);
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
}
*/

void MixerPage::onControlChangeCallback_Midi(uint8_t track, uint8_t track_param,
                                             uint8_t value) {
  if (track_param == 32) {
    redraw_mutes = true;
    return;
  } // don't process mute
  if (mixer_page.midi_device != &MD) {
    return;
  }
  SET_BIT16(mixer_page.redraw_mask, track);
  for (uint8_t i = 0; i < 16; i++) {
    if (note_interface.is_note_on(i) && (i != track)) {
      MD.setTrackParam(i, track_param, value, nullptr, true);
      SET_BIT16(mixer_page.redraw_mask, i);
    }
  }
  mixer_page.set_display_mode(track_param);
}

