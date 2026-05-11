#include "LFOPage.h"
#include "LFO.h"
#include "../Drivers/MD/MD.h"
#include "MCLGUI.h"
#include "ResourceManager.h"
#include "DeviceManager.h"
#include "DeviceParamTargets.h"
#include "../Drivers/MidiDevice.h"
#include "../Drivers/A4/A4.h"
#include "SeqPages.h"
#include "SeqTrackUtil.h"

#define LFO_TYPE 0
#define LFO_PARAM 1
#define INTERPOLATE
#define DIV_1_127 .0079

#define LFO_OFFSET 2
#define LFO_DESTINATION 1
#define LFO_SETTINGS 0

namespace {

const char *lfo_mode_label(uint8_t mode) {
  switch (LFOSeqTrack::mode_base(mode)) {
  case LFO_MODE_TRIG:
    return "TRIG";
  case LFO_MODE_ONE:
    return "ONE";
  case LFO_MODE_TRACK_TRIG:
    return "TRK";
  default:
    return "FREE";
  }
}

void update_lfo_key_interface(LFOSeqTrack *track) {
  uint8_t mode = track->base_mode();
  if (mode == LFO_MODE_TRIG || mode == LFO_MODE_ONE) {
    key_interface.on();
  } else {
    key_interface.off();
  }
}

} // namespace

void LFOPage::setup() {
  //  lfo_track = &mcl_seq.grid_x_lfo_tracks[0];

  DEBUG_PRINT_FN();
}

void LFOPage::track_update() {
  grid_x_tracks = SeqPage::current_device_slot() == 1;
#ifndef EXT_TRACKS
  grid_x_tracks = true;
#endif

  if (grid_x_tracks) {
    current_track = last_md_track;
  } else {
    current_track = last_ext_track;
  }
  lfo_track = &SeqTrackUtil::get_lfo_track(grid_x_tracks, current_track);
}

void LFOPage::sync_lfo_track() {
  MD.sync_seqtrack(lfo_track->length, lfo_track->speed,
                   lfo_track->step_count);
}

void LFOPage::select_menu_track(uint8_t track) {
  if (grid_x_tracks) {
    if (track >= NUM_GRID_X_LFO_TRACKS) {
      return;
    }
    last_md_track = track;
  } else {
#ifdef EXT_TRACKS
    if (track >= NUM_GRID_Y_LFO_TRACKS) {
      return;
    }
    last_ext_track = track;
#endif
  }
  track_update();
  sync_lfo_track();
  config_encoders();
}

void LFOPage::init() {
  MD.set_key_repeat(0);
  display_page_index = false;
  SeqPage::init();
  track_update();

  seq_menu_page.menu.enable_entry(SEQ_MENU_DEVICE, true);
  seq_menu_page.menu.enable_entry(SEQ_MENU_TRACK, true);
  seq_menu_page.menu.enable_entry(SEQ_MENU_SPEED, true);
  seq_menu_page.menu.enable_entry(SEQ_MENU_LENGTH_MD, true);
  seq_menu_page.menu.enable_entry(SEQ_MENU_LENGTH_EXT, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_COPY, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_CLEAR_TRACK, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_CLEAR_LOCKS, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_PASTE, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_SHIFT, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_REVERSE, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_TRANSPOSE, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_QUANT, false);
  seq_menu_page.menu.enable_entry(SEQ_MENU_LFO_MULT, true);

  sync_lfo_track();
  update_lfo_key_interface(lfo_track);


}

void LFOPage::cleanup() {
  SeqPage::cleanup();
  PerfPageParent::cleanup();
  key_interface.off();
}

void LFOPage::config_encoder_range(uint8_t i) {
  uint8_t device_slot = grid_x_tracks ? 1 : 2;
  ((MCLEncoder *)encoders[i])->max =
      DeviceParamTargets::slot_target_count(device_slot);
  uint8_t param_count =
      DeviceParamTargets::slot_param_count(device_slot, encoders[i]->cur);
  ((MCLEncoder *)encoders[i + 1])->max = param_count ? param_count - 1 : 0;
}

void LFOPage::config_encoders() {
  if (show_seq_menu) {
    return;
  }
  track_update();
  if (page_mode == LFO_DESTINATION) {
    encoders[0]->cur = lfo_track->params[0].dest;
    ((MCLEncoder *)encoders[0])->max =
        DeviceParamTargets::slot_target_count(grid_x_tracks ? 1 : 2);
    encoders[1]->cur = lfo_track->params[0].param;
    ((MCLEncoder *)encoders[1])->max = 23;
    encoders[2]->cur = lfo_track->params[1].dest;
    ((MCLEncoder *)encoders[2])->max =
        DeviceParamTargets::slot_target_count(grid_x_tracks ? 1 : 2);
    encoders[3]->cur = lfo_track->params[1].param;
    ((MCLEncoder *)encoders[3])->max = 23;

    config_encoder_range(0);
    config_encoder_range(2);
  }
  else if (page_mode == LFO_SETTINGS) {
    encoders[0]->cur = lfo_track->wav_type;
    ((MCLEncoder *)encoders[0])->max = LFO_WAV_COUNT - 1;

    encoders[1]->cur = lfo_track->speed;
    ((MCLEncoder *)encoders[1])->max = 127;

    encoders[2]->cur = lfo_track->params[0].depth;
    ((MCLEncoder *)encoders[2])->max = 127;

    encoders[3]->cur = lfo_track->params[1].depth;
    ((MCLEncoder *)encoders[3])->max = 127;
  }
  else if (page_mode == LFO_OFFSET) {
    encoders[0]->cur = lfo_track->base_mode();
    ((MCLEncoder *)encoders[0])->max = LFO_MODE_TRACK_TRIG;

    encoders[2]->cur = lfo_track->params[0].offset;
    ((MCLEncoder *)encoders[2])->max = 127;

    encoders[3]->cur = lfo_track->params[1].offset;
    ((MCLEncoder *)encoders[3])->max = 127;
  }

  //  loop();

  init_encoders_used_clock();

}

void LFOPage::loop() {
  track_update();
  if (show_seq_menu) {
    SeqPage::loop();
    uint8_t max_track = grid_x_tracks ? NUM_GRID_X_LFO_TRACKS
                                      : NUM_GRID_Y_LFO_TRACKS;
    if (opt_trackid > max_track) {
      opt_trackid = max_track;
      seq_menu_value_encoder.cur = opt_trackid;
    }
    return;
  }
  if (page_mode == LFO_DESTINATION) {
    config_encoder_range(0);
    config_encoder_range(2);

    if (encoders[0]->hasChanged()) {
      lfo_track->params[0].dest = encoders[0]->cur;
    }
    if (encoders[1]->hasChanged()) {
      lfo_track->params[0].param = encoders[1]->cur;
    }

    if (encoders[2]->hasChanged()) {
      lfo_track->params[1].dest = encoders[2]->cur;
    }
    if (encoders[3]->hasChanged()) {
      lfo_track->params[1].param = encoders[3]->cur;
    }
  }
  // wav_tables need to be recalculated when depth or waveform changes.

  else if (page_mode == LFO_SETTINGS) {
    if (encoders[0]->hasChanged()) {
      lfo_track->set_wav_type(encoders[0]->cur);
    }

    if (encoders[1]->hasChanged()) {
      lfo_track->set_speed(encoders[1]->cur);
    }

    if (encoders[2]->hasChanged()) {
      lfo_track->set_depth(0, encoders[2]->cur);
    }

    if (encoders[3]->hasChanged()) {
      lfo_track->set_depth(1, encoders[3]->cur);
    }
  }

  else if (page_mode == LFO_OFFSET) {
    if (encoders[0]->hasChanged()) {
      lfo_track->set_mode(encoders[0]->cur);
      update_lfo_key_interface(lfo_track);
    }

    if (encoders[2]->hasChanged()) {
      uint8_t value = 0;
      if (!DeviceParamTargets::slot_get_param(grid_x_tracks ? 1 : 2,
                                              lfo_track->params[0].dest,
                                              lfo_track->params[0].param,
                                              &value)) {
        lfo_track->params[0].offset = encoders[2]->cur;
      }
      else { encoders[2]->cur = encoders[2]->old; }
    }

    if (encoders[3]->hasChanged()) {
      uint8_t value = 0;
      if (!DeviceParamTargets::slot_get_param(grid_x_tracks ? 1 : 2,
                                              lfo_track->params[1].dest,
                                              lfo_track->params[1].param,
                                              &value)) {
         lfo_track->params[1].offset = encoders[3]->cur;
       }
       else { encoders[3]->cur = encoders[3]->old; }
    }
  }

}

void LFOPage::display() {
  track_update();
  oled_display.clearDisplay();

  uint8_t x = mcl_gui.knob_x0 + 5;
  uint8_t y = 8;
  uint8_t lfo_height = 7;
  uint8_t width = 13;

  // mcl_gui.draw_vertical_dashline(x, 0, knob_y);
  SeqPage::draw_knob_frame();
  const char *panel_info1;
  const char *panel_info2;


  if (page_mode == LFO_DESTINATION) {
    uint8_t device_slot = grid_x_tracks ? 1 : 2;
    draw_dest(0, encoders[0]->cur, true, device_slot);
    draw_param(1, encoders[0]->cur, encoders[1]->cur, device_slot);
    draw_dest(2, encoders[2]->cur, true, device_slot);
    draw_param(3, encoders[2]->cur, encoders[3]->cur, device_slot);
    panel_info2 = "LFO>DST";
  }
  else if (page_mode == LFO_SETTINGS) {
    uint8_t inc = LFO_LENGTH / width;
    for (uint8_t n = 0; n < LFO_LENGTH; n += inc, x++) {
      if (n < LFO_LENGTH) {
        uint16_t phase = ((uint16_t)n << 8) & LFO_PHASE_MASK;
        uint8_t out = LFOSeqTrack::get_preview_value(lfo_track->wav_type,
                                                     phase);
        uint8_t sample = ((int16_t)out * (int16_t)lfo_height) / 128;

        oled_display.drawPixel(x, y + lfo_height - sample, WHITE);
      }
    }
    x = mcl_gui.knob_x0 + 2;
    oled_display.setCursor(x + 5, 6);
    mcl_print_P(mclstr_wav_label);

    draw_knob(1, encoders[1], mclstr_spd);
    draw_knob(2, encoders[2], mclstr_dep1);
    draw_knob(3, encoders[3], mclstr_dep2);
    panel_info2 = "LFO>MOD";
  }
  else { //if (page_mode == LFO_OFFSET) {
    mcl_gui.draw_knob(0, mclstr_mode, lfo_mode_label(encoders[0]->cur));
    draw_knob(2, encoders[2], mclstr_ofs1);
    draw_knob(3, encoders[3], mclstr_ofs2);
    panel_info2 = "LFO>OFS";
  }

  oled_display.setFont(&TomThumb);

  const uint64_t slide_mask = 0;
  const uint64_t mute_mask = 0;

  panel_info1 = lfo_track->enable ? lfo_mode_label(lfo_track->mode) : "OFF";

  uint8_t base_mode = lfo_track->base_mode();
  if (base_mode == LFO_MODE_TRIG || base_mode == LFO_MODE_ONE) {
    draw_lock_mask(0, 0, lfo_track->step_count, lfo_track->length, true);
    draw_mask(0, lfo_track->pattern_mask, lfo_track->step_count,
              lfo_track->length, mute_mask, slide_mask, true);
    if ((uint16_t)lfo_track->pattern_mask != trigled_mask) {
      trigled_mask = (uint16_t)lfo_track->pattern_mask;
      mcl_gui.set_trigleds(lfo_track->pattern_mask, TRIGLED_STEPEDIT);
    }
  }

  strncpy(info1, panel_info1, sizeof(info1) - 1);
  info1[sizeof(info1) - 1] = '\0';
  strncpy(info2, panel_info2, sizeof(info2) - 1);
  info2[sizeof(info2) - 1] = '\0';
  SeqPage::display();
  draw_page_index(false, lfo_track->step_count / 4);

}

void LFOPage::capture_seq_menu_values(bool is_md_device) {
  (void)is_md_device;
  track_update();
  opt_trackid = current_track + 1;
  opt_speed = lfo_track->speed;
  opt_lfo_mult = lfo_track->speed_multiplier();
  opt_length = lfo_track->length ? lfo_track->length : 16;
  opt_channel = 0;
}

void LFOPage::apply_seq_menu_values(bool same_slot) {
  track_update();
  if (!same_slot) {
    return;
  }
  lfo_track->set_speed(opt_speed);
  lfo_track->set_speed_multiplier(opt_lfo_mult);
  if (opt_length == 0) {
    opt_length = 1;
  }
  if (opt_length > 64) {
    opt_length = 64;
  }
  lfo_track->length = opt_length;
  sync_lfo_track();
}

bool LFOPage::apply_seq_menu_row(uint8_t row_entry, void (*row_func)()) {
  switch (row_entry) {
  case SEQ_MENU_TRACK:
    select_menu_track(opt_trackid ? opt_trackid - 1 : 0);
    return true;
  case SEQ_MENU_DEVICE:
  case SEQ_MENU_SPEED:
  case SEQ_MENU_LFO_MULT:
  case SEQ_MENU_LENGTH_MD:
  case SEQ_MENU_LENGTH_EXT:
    return true;
  default:
    return SeqPage::apply_seq_menu_row(row_entry, row_func);
  }
}

void LFOPage::learn_param(uint8_t track, uint8_t param, uint8_t value) {
  learn_param(1, track + 1, param, value);
}

void LFOPage::learn_param(uint8_t device_slot, uint8_t dest, uint8_t param,
                          uint8_t value) {
  track_update();
  if (lfo_track->device_slot != device_slot) {
    return;
  }
  if (DeviceParamTargets::slot_param_count(device_slot, dest) <= param) {
    return;
  }
  bool reconfig = false;
  if (mcl.currentPage() == LFO_PAGE) {
  if (page_mode == LFO_DESTINATION) {
    if (encoders[0]->cur == 0 && encoders[1]->cur > 0) {
      lfo_track->params[0].dest = dest;
      lfo_track->params[0].param = param;
      reconfig = true;
    }
    if (encoders[2]->cur == 0 && encoders[3]->cur > 0) {
      lfo_track->params[1].dest = dest;
      lfo_track->params[1].param = param;
      reconfig = true;
    }
  }
  }
  if (lfo_track->params[0].dest == dest &&
      lfo_track->params[0].param == param) {
    lfo_track->params[0].offset = value;
    reconfig = true;
  }
  if (lfo_track->params[1].dest == dest &&
      lfo_track->params[1].param == param) {
    lfo_track->params[1].offset = value;
    reconfig = true;
  }
  if (mcl.currentPage() == LFO_PAGE && reconfig) { config_encoders(); }
}


bool LFOPage::handleEvent(gui_event_t *event) {
  if (PerfPageParent::handleEvent(event)) { return true; }

  bool seq_menu_button = EVENT_BUTTON(event) &&
      (EVENT_PRESSED(event, Buttons.BUTTON3) ||
       EVENT_RELEASED(event, Buttons.BUTTON3));
  if (show_seq_menu || seq_menu_button) {
    if (SeqPage::handleEvent(event)) {
      return true;
    }
    if (show_seq_menu) {
      return true;
    }
  }

  uint8_t base_mode = lfo_track->base_mode();
  if (EVENT_NOTE(event) &&
      (base_mode == LFO_MODE_TRIG || base_mode == LFO_MODE_ONE)) {
    uint8_t port = event->port;
    auto device = device_manager.device_for_port(port);

    uint8_t track = event->source;
    uint8_t page_select = 0;
    uint8_t step = track + (page_select * 16);
    if (event->mask == EVENT_BUTTON_PRESSED) {
      if (device == &Analog4) {
        // mcl.setPage(SEQ_EXTSTEP_PAGE)
        return true;
      }
      if (!IS_BIT_SET64(lfo_track->pattern_mask, step)) {

        SET_BIT64(lfo_track->pattern_mask, step);
      } else {
        DEBUG_PRINTLN(F("Trying to clear"));
        if (clock_diff(note_interface.note_hold[port], read_clock_ms()) <
            TRIG_HOLD_TIME) {
          CLEAR_BIT64(lfo_track->pattern_mask, step);
        }
      }
    }
  }
  if (EVENT_CMD(event)) {
    uint8_t key = event->source;
    if (event->mask == EVENT_BUTTON_PRESSED) {
      switch (key) {
      case MDX_KEY_YES: {
        goto lfo_enable;
      }
      case MDX_KEY_UP: {
        if (page_mode < LFO_OFFSET) {
          page_mode++;
          config_encoders();
        }
        return true;
      }
      case MDX_KEY_DOWN: {
        if (page_mode > 0) {
          page_mode--;
          config_encoders();
        }
        return true;
      }
      }
    }
  }
  if (EVENT_BUTTON(event)) {
    if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
      page_mode++;
      if (page_mode > LFO_OFFSET) { page_mode = 0; }
      config_encoders();
    }

    if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
      lfo_enable:
      lfo_track->enable = !(lfo_track->enable);
      if (!lfo_track->enable) { lfo_track->reset_params(); }
    }
  }
  return false;
}
