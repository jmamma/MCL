#include "MixerPage.h"
#include "DeviceManager.h"
#include "CommonPages.h"
#include "DevicePanelRef.h"
#include "ResourceManager.h"
#include "MCLGUI.h"
#include "MCLSeq.h"
#include "MixerPerf.h"
#include "SeqPages.h"
#include "SeqTrackUtil.h"

#define FADER_LEN 18
#define FADE_RATE 8

namespace {

uint16_t track_mask_for_len(uint8_t len) {
  if (len >= 16) {
    return 0xFFFF;
  }
  return (uint16_t)((1u << len) - 1u);
}

uint8_t mixer_param_to_7bit(const MidiDeviceMixerParam &param) {
#if defined(__AVR__)
  return param.value;
#else
  if (param.max_value <= param.min_value) {
    return 0;
  }

  int32_t value = param.value;
  if (value < param.min_value) value = param.min_value;
  if (value > param.max_value) value = param.max_value;
  const uint16_t range = (uint16_t)(param.max_value - param.min_value);
  return (uint8_t)(((uint32_t)(value - param.min_value) * 127u +
                    (range / 2u)) /
                   range);
#endif
}

PageIndex mixer_return_page(PageIndex &last_page) {
  PageIndex page = last_page != NULL_PAGE ? last_page : GRID_PAGE;
  last_page = NULL_PAGE;
  return page;
}

} // namespace

MidiDevice *MixerPage::device_for_mixer_idx(uint8_t device_idx) const {
  return device_manager.device_for_idx(device_idx);
}

DeviceContext MixerPage::context_for_mixer_idx(uint8_t device_idx) const {
  return device_manager.context_for_device(device_idx);
}

DeviceContext MixerPage::selected_mixer_context() const {
  return context_for_mixer_idx(mixer_device_idx);
}

MidiDevice *MixerPage::selected_mixer_device() const {
  return device_for_mixer_idx(mixer_device_idx);
}

void MixerPage::sync_selected_mixer_device() {
  midi_device = selected_mixer_device();
  if (midi_device == &null_midi_device) {
    mixer_device_idx = 0;
    midi_device = device_manager.primary_device();
  }
  if (midi_device == &null_midi_device) {
    midi_device = &MD;
  }
}

void MixerPage::select_mixer_device(uint8_t device_idx) {
  mixer_device_idx = device_idx & 1;
  sync_selected_mixer_device();
  set_display_mode(default_mixer_param());
  redraw();
}

uint8_t MixerPage::default_mixer_param() const {
  DeviceContext ctx = selected_mixer_context();
  return ctx.device()->mixer()->default_param(ctx);
}

uint8_t MixerPage::mixer_track_count() const {
  DeviceContext ctx = selected_mixer_context();
  uint8_t count = ctx.device()->mixer()->track_count(ctx);
  return count > 16 ? 16 : count;
}

SeqTrack *MixerPage::mixer_seq_track(uint8_t track) const {
  DeviceContext ctx = selected_mixer_context();
  return ctx.device()->mixer()->seq_track(ctx, track);
}

TrigLEDMode MixerPage::mixer_led_mode() const {
  MidiDevice *device = selected_mixer_device();
  bool is_md_device = SeqTrackUtil::is_md_device(device);
  return is_md_device ? TRIGLED_OVERLAY : TRIGLED_EXCLUSIVE;
}

uint8_t *MixerPage::mixer_meter_levels() {
  return mixer_device_idx == 0 ? disp_levels : ext_disp_levels;
}

void MixerPage::track_trig(uint8_t device_idx, uint8_t track_number,
                           uint8_t level) {
  if (track_number >= 16) {
    return;
  }
  if (device_idx == 1) {
    ext_disp_levels[track_number] = level;
  } else {
    disp_levels[track_number] = level;
  }
#ifdef LFO_TRACKS
  mcl_seq.set_lfo_track_trig(device_idx, track_number);
#endif
}

void MixerPage::trig(uint8_t track_number) {
  if (track_number >= NUM_MD_TRACKS) {
    return;
  }
  DeviceContext ctx = device_manager.primary_context();
  DeviceMixerCapability *mixer = ctx.device()->mixer();
  MidiDeviceMixerParam info;
  uint8_t level = 127;
  if (mixer->param(ctx, track_number, mixer->default_param(ctx), &info)) {
    level = mixer_param_to_7bit(info);
  }
  track_trig(1, track_number, level);
  GUI_hardware.led.set_flashled(track_number);

  uint8_t trig_group = mixer->trig_group(ctx, track_number);
  if (trig_group < NUM_MD_TRACKS) {
    level = 127;
    if (mixer->param(ctx, trig_group, mixer->default_param(ctx), &info)) {
      level = mixer_param_to_7bit(info);
    }
    track_trig(1, trig_group, level);
    GUI_hardware.led.set_flashled(trig_group);
  }
}

bool MixerPage::mixer_param_supported_for_held_tracks(uint8_t param) {
  sync_selected_mixer_device();
  DeviceContext ctx = selected_mixer_context();
  DeviceMixerCapability *mixer = midi_device->mixer();
  uint8_t len = mixer_track_count();
  for (uint8_t i = 0; i < len; i++) {
    if (note_interface.is_note_on(i)) {
      MidiDeviceMixerParam info;
      if (mixer->param(ctx, i, param, &info)) {
        return true;
      }
    }
  }
  return false;
}

uint8_t MixerPage::mixer_param_for_encoder(uint8_t encoder_idx,
                                           bool is_md_device) {
  if (is_md_device) {
    return MixerPerf::mixer_param_for_encoder(encoder_idx);
  }

#if defined(__AVR__)
  return default_mixer_param();
#else
  if (mixer_param_supported_for_held_tracks(encoder_idx)) {
    return encoder_idx;
  }
  if (mixer_param_supported_for_held_tracks(display_mode)) {
    return display_mode;
  }
  return default_mixer_param();
#endif
}

bool MixerPage::handle_mixer_encoder_edits(bool is_md_device) {
  if (note_interface.notes_on == 0) {
    return false;
  }

  bool handled = false;
  for (uint8_t n = 0; n < GUI_NUM_ENCODERS; n++) {
    if (encoders[n]->hasChanged()) {
      adjust_param(encoders[n], mixer_param_for_encoder(n, is_md_device));
      handled = true;
    }
  }
  return handled;
}

bool MixerPage::display_mute_mask() {
  uint16_t last_mute_mask = seq_step_page.mute_mask;
  seq_step_page.mute_mask = 0;

  uint8_t len = mixer_track_count();
  for (uint8_t i = 0; i < len; i++) {
    SeqTrack *seq_track = mixer_seq_track(i);
    if (seq_track != nullptr && seq_track->mute_state == SEQ_MUTE_OFF) {
      SET_BIT16(seq_step_page.mute_mask, i);
    }
  }

  if (last_mute_mask != seq_step_page.mute_mask) {
    MidiDevice *device = selected_mixer_device();
    TrigLEDMode led_mode =
        SeqTrackUtil::is_md_device(device) ? TRIGLED_MUTE : TRIGLED_EXCLUSIVE;
    mcl_gui.set_trigleds(seq_step_page.mute_mask, led_mode);
    return true;
  }
  return false;
}

void MixerPage::set_display_mode(uint8_t param) {
  if (display_mode != param) {
    redraw_mask = -1;
    display_mode = param;
  }
}

void MixerPage::oled_draw_mutes() {
  sync_selected_mixer_device();
  uint8_t len = mixer_track_count();
  uint8_t fader_x = 0;
  uint8_t slot = mixer_device_idx;

  bool draw = true;
  if (preview_mute_set != 255 && load_types[preview_mute_set][slot] == 0) {
    draw = false;
  }
  for (uint8_t i = 0; i < len; ++i) {
    SeqTrack *seq_track = mixer_seq_track(i);
    if (seq_track == nullptr) {
      fader_x += 8;
      continue;
    }

    uint8_t mute_state =
        preview_mute_set != 255
            ? IS_BIT_SET16(mute_sets[slot].mutes[preview_mute_set], i)
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
#ifdef PLATFORM_TBD
  device_manager.exit_ui();
#endif
  sync_selected_mixer_device();
  level_pressmode = 0;
  /*
  for (uint8_t i = 0; i < 4; i++) {
    encoders[i]->cur = 64;
    encoders[i]->old = 64;
  }
  */
  DevicePanelRef::set_primary_key_repeat(0);
  key_interface.on();
  mcl_gui.set_trigleds(0, mixer_led_mode());
  preview_mute_set = 255;
  oled_display.clearDisplay();
  set_display_mode(default_mixer_param());
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
  DevicePanelRef::set_primary_key_repeat(1);
  disable_record_mutes();
  key_interface.off();
  ext_key_down = 0;
  mute_toggle = 0;
}

void MixerPage::set_level(int curtrack, int value) {
  sync_selected_mixer_device();
  DeviceContext ctx = selected_mixer_context();
  DeviceMixerCapability *mixer = midi_device->mixer();
  mixer->set_param(ctx, curtrack, mixer->default_param(ctx), value, true);
}

void MixerPage::load_perf_locks(uint8_t state) {
  MixerPerf::load_locks(encoders, perf_locks, state);
}
void MixerPage::loop() {
  constexpr int timeout = 750;
  bool old_draw_encoders = draw_encoders;
  sync_selected_mixer_device();
  const bool use_perf_encoders = MixerPerf::available(midi_device);
  const bool notes_on = note_interface.notes_on;
  bool mixer_encoder_edit = notes_on &&
                            handle_mixer_encoder_edits(use_perf_encoders);

  if (use_perf_encoders && !mixer_encoder_edit) {
    MixerPerf::func_enc_check();

    MixerPerf::handle_preview_lock_edits(encoders, perf_locks,
                                         preview_mute_set, notes_on);

    MixerPerf::encoder_send();

    if (!(draw_encoders && key_interface.is_key_down(MDX_KEY_FUNC))) {
      draw_encoders = false;
      uint64_t mask =
          ((uint64_t)1 << MDX_KEY_LEFT) | ((uint64_t)1 << MDX_KEY_UP) |
          ((uint64_t)1 << MDX_KEY_RIGHT) | ((uint64_t)1 << MDX_KEY_DOWN) |
          ((uint64_t)1 << MDX_KEY_YES);
      for (uint8_t n = 0; n < 4; n++) {
        bool check = (key_interface.cmd_key_state & mask);

        if (MixerPerf::should_show_encoder(encoders[n], encoders_used_clock[n],
                                           notes_on || check, timeout)) {
          draw_encoders = true;
        }
      }
    }
  } else {
    draw_encoders = false;
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
  mixer_page.adjust_param(enc, mixer_page.default_mixer_param());
}

void MixerPage::draw_encs() {
  constexpr uint8_t fader_y = 11;
  oled_display.fillRect(0, fader_y, 128, 21, BLACK);
  for (uint8_t n = 0; n < 4; n++) {
    uint8_t pos = n * 24;
    bool highlight = false;
    uint8_t val = MixerPerf::display_value(encoders[n], perf_locks,
                                           preview_mute_set, n, highlight);
    mcl_gui.draw_encoder(24 + pos, fader_y + 4, val, highlight);
    oled_display.setCursor(16 + pos, fader_y + 6);
    oled_display.write((uint8_t)('A' + n));
  }
}

void MixerPage::adjust_param(EncoderParent *enc, uint8_t param) {
  sync_selected_mixer_device();
  DeviceContext ctx = selected_mixer_context();
  DeviceMixerCapability *mixer = midi_device->mixer();
  set_display_mode(param);

  int dir = enc->getValue() - enc->old;
  int16_t newval;

  uint8_t len = mixer_track_count();
  for (uint8_t i = 0; i < len; i++) {
    if (note_interface.is_note_on(i)) {
      MidiDeviceMixerParam info;
      if (mixer->param(ctx, i, param, &info)) {
        newval = info.value + dir;
#ifdef PLATFORM_TBD
        if (newval < info.min_value) newval = info.min_value;
        if (newval > info.max_value) newval = info.max_value;
#else
        if (newval < 0) newval = 0;
        if (newval > 127) newval = 127;
#endif
        if (mixer->set_param(ctx, i, param, newval, true)) {
          SET_BIT16(redraw_mask, i);
        }
      }
    }
  }
  enc->cur = 64 + dir;
  enc->old = 64;
}

void MixerPage::display() {
  sync_selected_mixer_device();
  DeviceMixerCapability *mixer = midi_device->mixer();

  if (oled_display.textbox_enabled) {
    redraw();
  }

  if (redraw_mutes) {
    oled_draw_mutes();
    redraw_mutes = false;
  }

  constexpr uint8_t fader_y = 11;

  uint8_t mute_set = preview_mute_set;
  uint8_t slot = mixer_device_idx;

  if (((ext_key_down && mute_set == 255) || show_mixer_menu) &&
      display_mute_mask()) {
    oled_draw_mutes();
  } else if (mute_set != 255 && mute_sets[slot].mutes[mute_set] !=
                                    seq_step_page.mute_mask) {

    uint16_t mask = mute_sets[slot].mutes[mute_set];
    if (load_types[mute_set][slot] == 0) { mask = 0; }
    mask &= track_mask_for_len(mixer_track_count());
    mcl_gui.set_trigleds(mask, mixer_led_mode());
    seq_step_page.mute_mask = mask;
    oled_draw_mutes();
  }
  const bool show_perf_encoders = MixerPerf::available(midi_device);
  if (show_perf_encoders &&
      (draw_encoders || preview_mute_set != 255)) {
    // oled_display.clearDisplay();
    draw_encs();
    oled_display.setFont(&TomThumb);
    if (load_mute_set != 255 && load_mute_set == preview_mute_set) {
      oled_display.setCursor(111, 31);
      mcl_print_P(mclstr_load);
    }
    oled_display.setFont();
  } else {

    uint8_t fader_level;
    uint8_t meter_level;
    uint8_t fader_x = 0;

    uint8_t len = mixer_track_count();
    uint8_t *levels = mixer_meter_levels();

    uint8_t dec = FADE_RATE;

    for (uint8_t i = 0; i < len; i++) {

      MidiDeviceMixerParam info;
      if (mixer->param(selected_mixer_context(), i, display_mode, &info)) {
        fader_level = mixer_param_to_7bit(info);
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
  sync_selected_mixer_device();
  DeviceContext ctx = selected_mixer_context();
  DeviceMixerCapability *mixer = midi_device->mixer();
  uint8_t len = mixer_track_count();
  for (uint8_t i = 0; i < len; i++) {
    if (note_interface.is_note_on(i)) {
      mixer->set_record_mutes(ctx, i, state, !state);
    }
  }
}

void MixerPage::disable_record_mutes(bool clear) {
  for (uint8_t dev = 0; dev < 2; dev++) {
    DeviceContext ctx = context_for_mixer_idx(dev);
    if (ctx.device() == &null_midi_device) {
      continue;
    }
    DeviceMixerCapability *mixer = ctx.device()->mixer();
    uint8_t len = mixer->track_count(ctx);
    if (len > 16) len = 16;
    for (uint8_t n = 0; n < len; n++) {
      mixer->set_record_mutes(ctx, n, false, clear);
    }
  }
  if (!seq_step_page.recording) {
    clearLed2();
  }
}

void MixerPage::switch_mute_set(uint8_t state, bool load_perf, bool *load_type) {
  if (load_type != nullptr && state < 255) {
    for (uint8_t dev = 0; dev < 2; dev++) {
      if (!load_type[dev]) continue;
      DeviceContext ctx = context_for_mixer_idx(dev);
      DeviceMixerCapability *mixer = ctx.device()->mixer();
      uint8_t len = mixer->track_count(ctx);
      if (len > 16) len = 16;

      for (uint8_t n = 0; n < len; n++) {
        SeqTrack *seq_track = mixer->seq_track(ctx, n);
        if (seq_track == nullptr) {
          continue;
        }

        bool mute_state = IS_BIT_CLEAR16(mute_sets[dev].mutes[state], n);
        // Flip
        if (state == 4 && dev == mixer_device_idx) {
          mute_state = !seq_track->mute_state;
        }
        // Switch
        if (mute_state != seq_track->mute_state) {
          mixer->mute_track(ctx, n, mute_state);
          seq_track->mute_state = mute_state;
        }
      }
    }
  }
  if (state < 4 && load_perf &&
      MixerPerf::available(selected_mixer_device())) {
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
  sync_selected_mixer_device();
  DeviceMixerCapability *mixer = midi_device->mixer();
  uint8_t len = mixer_track_count();
  for (uint8_t i = 0; i < len; i++) {
    bool note_on = note_interface.is_note_on(i);
    SeqTrack *seq_track = mixer_seq_track(i);
    if (seq_track == nullptr) {
      continue;
    }

    if (solo) {
      bool mute_state = !note_on;
      if (seq_track->mute_state != mute_state) {
        seq_track->mute_state = mute_state;
        mixer->mute_track(selected_mixer_context(), i, mute_state);
      }
    } else if (note_on) {
      // TOGGLE
      seq_track->toggle_mute();
      mixer->mute_track(selected_mixer_context(), i, seq_track->mute_state);
    }
  }
  oled_draw_mutes();
}

bool MixerPage::handleEvent(gui_event_t *event) {

  sync_selected_mixer_device();
  bool use_perf = MixerPerf::available(midi_device);
  DeviceMixerCapability *mixer = midi_device->mixer();
  uint8_t slot = mixer_device_idx;
  if (EVENT_NOTE(event)) {
    uint8_t track = event->source;

    if (track >= 16) {
      return false;
    }
    if (!ext_key_down && !show_mixer_menu && preview_mute_set == 255) {
      key_interface.send_md_leds(mixer_led_mode());
    }

    uint8_t len = mixer_track_count();
    if (event->mask == EVENT_BUTTON_PRESSED && track < len) {
      if (note_interface.is_note(track)) {
        if (show_mixer_menu || ext_key_down ||
            (preview_mute_set != 255 &&
             load_types[preview_mute_set][slot])) {
          if (ext_key_down) {
            mute_toggle = 1;
          }
          SeqTrack *seq_track = mixer_seq_track(track);
          if (seq_track == nullptr) {
            return true;
          }

          uint8_t mute_set = preview_mute_set;

          // Toggle active mutes
          if (mute_set == 255) {
            seq_track->toggle_mute();
            mixer->mute_track(selected_mixer_context(), track,
                              seq_track->mute_state);
            return true;
          }

          // Toggle preview mutes
          bool state =
              IS_BIT_SET16(mute_sets[slot].mutes[mute_set], track);
          if (state == SEQ_MUTE_ON) {
            CLEAR_BIT16(mute_sets[slot].mutes[mute_set], track);
          } else {
            SET_BIT16(mute_sets[slot].mutes[mute_set], track);
          }

          // oled_draw_mutes();
        } else if (first_track == 255 && use_perf) {
          first_track = track;
          mixer->select_track(selected_mixer_context(), track);
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
    uint8_t key = event->source;
    if (event->mask == EVENT_BUTTON_PRESSED) {
      switch (key) {
      case MDX_KEY_KIT: {
        record_mutes_set(false);
        // key_interface.ignore_next_event(GLOBAL);
        break;
      }
      case MDX_KEY_PATSONGKIT: {
        disable_record_mutes(true);
        // key_interface.ignore_next_event(GLOBAL);
        break;
      }
      case MDX_KEY_EXTENDED: {
        DEBUG_PRINTLN("key extended");
        ext_key_down = 1;
        redraw();
        if (use_perf) {
          DeviceContext restore_ctx = selected_mixer_context();
          for (uint8_t i = 0; i < 16; i++) {
            if (note_interface.is_note_on(i)) {
              mixer->restore_track_params(restore_ctx, i);
            }
          }
        }
        break;
      }
      case MDX_KEY_NO: {
        uint64_t mask =
            ((uint64_t)1 << MDX_KEY_LEFT) | ((uint64_t)1 << MDX_KEY_UP) |
            ((uint64_t)1 << MDX_KEY_RIGHT) | ((uint64_t)1 << MDX_KEY_DOWN);
        if (((key_interface.cmd_key_state & mask) == 0)) {
          if (note_interface.notes_count_on() == 0) {
            mcl.setPage(mixer_return_page(last_page));
            return true;
          }
          toggle_or_solo(true);
        }
        break;
      }
      case MDX_KEY_BANKA: {
        if (preview_mute_set != 255) {
          load_types[preview_mute_set][slot] =
              !load_types[preview_mute_set][slot];
          if (load_types[preview_mute_set][slot] == 0) {
            seq_step_page.mute_mask = 0;
          }
          redraw_mutes = true;
          return true;
        }
        break;
      }
      case MDX_KEY_BANKB: {
        if (preview_mute_set != 255) {
          if (load_mute_set == preview_mute_set) {
            load_mute_set = 255;
          } else {
            load_mute_set = preview_mute_set;
          }
          return true;
        }
        break;
      }
      case MDX_KEY_YES: {
        if (preview_mute_set == 255 &&
            key_interface.is_key_down(MDX_KEY_FUNC) &&
            note_interface.notes_on == 0) {
          bool load_t[2] = {0, 0};
          load_t[slot] = 1;
          switch_mute_set(4, false, load_t); //---> Flip mutes
          break;
        }
        uint8_t set = 255;

        if (key_interface.is_key_down(MDX_KEY_LEFT)) {
          set = 1;
        } else if (key_interface.is_key_down(MDX_KEY_UP)) {
          set = 2;
        } else if (key_interface.is_key_down(MDX_KEY_RIGHT)) {
          set = 3;
        } else if (key_interface.is_key_down(MDX_KEY_DOWN)) {
          set = 0;
        }
        if (set != 255) {
          // bool load_t[2];
          // load_t[0] = load_types[key][0];
          // load_t[1] = load_types[key][1];
          // load_t[is_md_device] = 0;
          switch_mute_set(set, true, load_types[set]);
        } else {
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
        if (key_interface.is_key_down(MDX_KEY_NO)) {
          return true;
        }
        uint8_t set = get_mute_set(key);
        if (key_interface.is_key_down(MDX_KEY_YES)) {
          switch_mute_set(set, true, load_types[set]);
        } else {
          preview_mute_set = set;
          // force redraw in display()
          seq_step_page.mute_mask++;
        }
        break;
      }
      case MDX_KEY_SCALE: {
        select_mixer_device(mixer_device_idx == 0 ? 1 : 0);
        key_interface.send_md_leds(mixer_led_mode());
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
          mcl.setPage(mixer_return_page(last_page));
          return true;
        }
        mute_toggle = 0;
        if (!show_mixer_menu && preview_mute_set == 255) {
          key_interface.send_md_leds(mixer_led_mode());
        }
        return true;
      }
      case MDX_KEY_LEFT:
      case MDX_KEY_UP:
      case MDX_KEY_RIGHT:
      case MDX_KEY_DOWN: {
        uint64_t mask =
            ((uint64_t)1 << MDX_KEY_LEFT) | ((uint64_t)1 << MDX_KEY_UP) |
            ((uint64_t)1 << MDX_KEY_RIGHT) | ((uint64_t)1 << MDX_KEY_DOWN) |
            ((uint64_t)1 << MDX_KEY_YES);
        if ((key_interface.cmd_key_state & mask) == 0) {
          key_interface.send_md_leds(mixer_led_mode());
          preview_mute_set = 255;
          redraw();
        }
        break;
      }
      }
    }
  }
  if (EVENT_BUTTON(event)) {
    if (EVENT_RELEASED(event, Buttons.BUTTON3)) {
    global_release:
      preview_mute_set = 255;
      show_mixer_menu = false;
      disable_record_mutes();
      mcl_gui.set_trigleds(0, mixer_led_mode());
      redraw();
      return true;
    }
    if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
      // show_mixer_menu = true;
      if (note_interface.notes_on) {
        setLed2();
        record_mutes_set(true);
        return true;
      }
      if (use_perf) {
        for (uint8_t n = 0; n < GUI_NUM_ENCODERS; n++) {
          if (BUTTON_DOWN(Buttons.ENCODER1 + n)) {
            MixerPerf::clear_scenes(encoders[n]);
          }
        }
      }
      redraw_mask = -1;
      return true;
    }

    if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
      if (use_perf) {
        for (uint8_t n = 0; n < GUI_NUM_ENCODERS; n++) {
          if (BUTTON_DOWN(Buttons.ENCODER1 + n)) {
            MixerPerf::scene_autofill(encoders[n]);
          }
        }
      }
      redraw_mask = -1;
      return true;
    }
    if (use_perf && preview_mute_set != 255 &&
        (key_interface.is_key_down(MDX_KEY_NO))) {
      if (event->source >= Buttons.ENCODER1 &&
          event->source <= Buttons.ENCODER4) {
        uint8_t b = event->source - Buttons.ENCODER1;
        bool pressed = ((event)->mask & EVENT_BUTTON_PRESSED) != 0;
        MixerPerf::handle_preview_lock_button(encoders[b], perf_locks,
                                              preview_mute_set, b, pressed);
        return true;
      }
    }
  }
  return false;
}
void MixerPage::onControlChangeCallback_Midi(uint8_t device_idx,
                                             uint8_t track,
                                             uint8_t track_param,
                                             uint8_t value) {
  if (device_idx == 255) {
    return;
  }
  DeviceContext ctx = device_manager.context_for_device(device_idx);
  DeviceMixerCapability *mixer = ctx.device()->mixer();
  if (mixer->is_mute_param(track_param)) {
    if (device_idx == mixer_device_idx) {
      redraw_mutes = true;
    }
    return;
  }
  if (device_idx != mixer_device_idx) {
    return;
  }
  sync_selected_mixer_device();
  SET_BIT16(mixer_page.redraw_mask, track);
  uint8_t len = mixer_page.mixer_track_count();
  for (uint8_t i = 0; i < len; i++) {
    if (note_interface.is_note_on(i) && (i != track)) {
      if (mixer->set_param(ctx, i, track_param, value, true)) {
        SET_BIT16(mixer_page.redraw_mask, i);
      }
    }
  }
  mixer_page.set_display_mode(track_param);
}
