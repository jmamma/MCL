#include "LFOPage.h"
#include "LFO.h"
#include "MCLGUI.h"
#include "ResourceManager.h"
#include "SeqPages.h"

#define LFO_TYPE 0
#define LFO_PARAM 1
#define INTERPOLATE
#define DIV_1_127 .0079

#define LFO_OFFSET 2
#define LFO_DESTINATION 1
#define LFO_SETTINGS 0

namespace {

const char *lfo_mode_label(uint8_t mode) NOINLINE();
const char *lfo_mode_label(uint8_t mode) {
  switch (LFOSeqTrack::mode_base(mode)) {
  case LFO_MODE_TRIG:
    return "TRG";
  case LFO_MODE_ONE:
    return "ONE";
  case LFO_MODE_TRACK_TRIG:
    return "TRK";
  default:
    return "FRE";
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

void update_lfo_param_pair(Encoder **encoders, LFOSeqTrack *track,
                           uint8_t encoder_idx,
                           uint8_t param_idx) NOINLINE();
void update_lfo_param_pair(Encoder **encoders, LFOSeqTrack *track,
                           uint8_t encoder_idx, uint8_t param_idx) {
  if (encoders[encoder_idx]->hasChanged()) {
    track->params[param_idx].dest = encoders[encoder_idx]->cur;
  }
  ++encoder_idx;
  if (encoders[encoder_idx]->hasChanged()) {
    track->params[param_idx].param = encoders[encoder_idx]->cur;
  }
}

const char lfo_mult_label_1x[] PROGMEM = "1x";
const char lfo_mult_label_2x[] PROGMEM = "2x";
const char lfo_mult_label_4x[] PROGMEM = "4x";
const char lfo_mult_label_8x[] PROGMEM = "8x";
const char lfo_mult_label_half[] PROGMEM = "0.5";
const char lfo_mult_label_quarter[] PROGMEM = "0.25";
const char lfo_mult_label_tenth[] PROGMEM = "0.1";
const char lfo_mult_label_hundredth[] PROGMEM = "0.01";
const char *const lfo_mult_labels[LFO_SPEED_MULT_COUNT] PROGMEM = {
    lfo_mult_label_1x,      lfo_mult_label_2x,    lfo_mult_label_4x,
    lfo_mult_label_8x,      lfo_mult_label_half,  lfo_mult_label_quarter,
    lfo_mult_label_tenth,   lfo_mult_label_hundredth};

void lfo_mult_label(uint8_t multiplier, char *out) {
  if (multiplier >= LFO_SPEED_MULT_COUNT) {
    multiplier = LFO_SPEED_MULT_1X;
  }
  PGM_P label = (PGM_P)pgm_read_ptr(&lfo_mult_labels[multiplier]);
  strcpy_P(out, label);
}

bool lfo_preview_is_centered(uint8_t wav_type) {
  constexpr uint16_t centered_mask = (1 << TRI_WAV) | (1 << SAW_WAV) |
                                     (1 << SQU_WAV) | (1 << RND_WAV) |
                                     (1 << SIN_WAV);
  return (centered_mask >> wav_type) & 1;
}

void update_lfo_offset(Encoder **encoders, LFOSeqTrack *track,
                       uint8_t encoder_idx, uint8_t param_idx) NOINLINE();
void update_lfo_offset(Encoder **encoders, LFOSeqTrack *track,
                       uint8_t encoder_idx, uint8_t param_idx) {
  if (!encoders[encoder_idx]->hasChanged()) {
    return;
  }
  uint8_t value = 0;
  if (!LFOTrackRef::get_param(track->device_idx, track->params[param_idx].dest,
                              track->params[param_idx].param, &value)) {
    track->params[param_idx].offset = encoders[encoder_idx]->cur;
  } else {
    encoders[encoder_idx]->cur = encoders[encoder_idx]->old;
  }
}

} // namespace

void LFOPage::setup() {
  //  lfo_track = &mcl_seq.grid_x_lfo_tracks[0];

  DEBUG_PRINT_FN();
}

void LFOPage::track_update() {
  lfo_track = &LFOTrackRef::current_track();
}

void LFOPage::sync_lfo_track() {
  LFOTrackRef::sync_panel(*lfo_track);
}

void LFOPage::select_menu_track(uint8_t track) {
  if (!LFOTrackRef::select_track(track)) {
    return;
  }
  track_update();
  sync_lfo_track();
  config_encoders();
}

void LFOPage::init() {
  LFOTrackRef::set_key_repeat(0);
  display_page_index = false;
  SeqPage::init();
  track_update();

  constexpr uint32_t lfo_menu_entries =
      menu_entry_mask(SEQ_MENU_TRACK) | menu_entry_mask(SEQ_MENU_DEVICE) |
      menu_entry_mask(SEQ_MENU_SPEED) | menu_entry_mask(SEQ_MENU_LENGTH_MD) |
      menu_entry_mask(SEQ_MENU_LFO_MULT);
  seq_menu_page.menu.set_enabled_entry_mask(lfo_menu_entries);

  sync_lfo_track();
  update_lfo_key_interface(lfo_track);


}

void LFOPage::cleanup() {
  SeqPage::cleanup();
  PerfPageParent::cleanup();
  key_interface.off();
}

void LFOPage::config_encoder_range(uint8_t i) {
  ((MCLEncoder *)encoders[i])->max =
      LFOTrackRef::target_count(lfo_track->device_idx);
  uint8_t param_count =
      LFOTrackRef::param_count(lfo_track->device_idx, encoders[i]->cur);
  ((MCLEncoder *)encoders[i + 1])->max = param_count ? param_count - 1 : 0;
}

void LFOPage::config_encoders() {
  if (show_seq_menu) {
    return;
  }
  track_update();
  if (page_mode == LFO_DESTINATION) {
    encoders[0]->cur = lfo_track->params[0].dest;
    encoders[1]->cur = lfo_track->params[0].param;
    ((MCLEncoder *)encoders[1])->max = 23;
    encoders[2]->cur = lfo_track->params[1].dest;
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

    encoders[1]->cur = lfo_track->speed_multiplier();
    ((MCLEncoder *)encoders[1])->max = LFO_SPEED_MULT_COUNT - 1;

    encoders[2]->cur = lfo_track->params[0].offset;
    ((MCLEncoder *)encoders[2])->max = 127;

    encoders[3]->cur = lfo_track->params[1].offset;
    ((MCLEncoder *)encoders[3])->max = 127;
  }

  //  loop();

  init_encoders_used_clock();

}

bool LFOPage::moveEncoderFocusPage(int8_t direction) {
  if (show_seq_menu) {
    return false;
  }
  if (direction > 0) {
    page_mode = page_mode < LFO_OFFSET ? page_mode + 1 : 0;
  } else {
    page_mode = page_mode > 0 ? page_mode - 1 : LFO_OFFSET;
  }
  config_encoders();
  return true;
}

void LFOPage::loop() {
  track_update();
  if (show_seq_menu) {
    SeqPage::loop();
    uint8_t max_track = LFOTrackRef::track_count(lfo_track->device_idx);
    if (opt_trackid > max_track) {
      opt_trackid = max_track;
      seq_menu_value_encoder.cur = opt_trackid;
    }
    return;
  }
  if (page_mode == LFO_DESTINATION) {
    config_encoder_range(0);
    config_encoder_range(2);

    update_lfo_param_pair(encoders, lfo_track, 0, 0);
    update_lfo_param_pair(encoders, lfo_track, 2, 1);
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
    if (encoders[1]->hasChanged()) {
      lfo_track->set_speed_multiplier(encoders[1]->cur);
    }

    update_lfo_offset(encoders, lfo_track, 2, 0);
    update_lfo_offset(encoders, lfo_track, 3, 1);
  }

}

void LFOPage::display() {
  track_update();
  oled_display.clearDisplay();

  // mcl_gui.draw_vertical_dashline(x, 0, knob_y);
  SeqPage::draw_knob_frame();
  const char *panel_info1;
  const char *panel_info2;


  if (page_mode == LFO_DESTINATION) {
    DeviceIdx device_idx = lfo_track->device_idx;
    draw_dest(0, encoders[0]->cur, true, device_idx);
    draw_param(1, encoders[0]->cur, encoders[1]->cur, device_idx);
    draw_dest(2, encoders[2]->cur, true, device_idx);
    draw_param(3, encoders[2]->cur, encoders[3]->cur, device_idx);
    panel_info2 = "LFO>DST";
  }
  else if (page_mode == LFO_SETTINGS) {
    const uint8_t preview_x = mcl_gui.knob_x0 + 5;
    const uint8_t preview_y = 10;
    const uint8_t preview_height = 7;
    const uint8_t preview_width = 15;

    oled_display.setFont(&TomThumb);
    oled_display.setTextColor(WHITE);
    oled_display.setCursor(mcl_gui.knob_x0 + 5, mcl_gui.knob_y0 + 6);
    mcl_print_P(mclstr_wav_label);
    oled_display.setFont();

    uint8_t wav_type =
        lfo_track->wav_type < LFO_SHAPE_COUNT ? lfo_track->wav_type : TRI_WAV;
    uint8_t ref_y = lfo_preview_is_centered(wav_type)
                        ? preview_y + 4
                        : preview_y + preview_height;
    for (uint8_t i = 0; i < preview_width; i += 2) {
      oled_display.drawPixel(preview_x + i, ref_y, WHITE);
    }

    uint16_t phase = 0;
    const uint16_t phase_step = LFO_PHASE_MASK / (preview_width - 1);
    for (uint8_t i = 0; i < preview_width; ++i, phase += phase_step) {
      uint8_t out =
          LFOSeqTrack::get_preview_value(wav_type, phase & LFO_PHASE_MASK);
      uint8_t y;
      if (wav_type == RND_WAV) {
        uint8_t band = out & 1;
        y = out < 64 ? preview_y + preview_height - band : preview_y + band;
      } else {
        uint8_t sample = ((uint16_t)out * preview_height) >> 7;
        y = preview_y + preview_height - sample;
      }
      oled_display.drawPixel(preview_x + i, y, WHITE);
    }

    draw_knob(1, encoders[1], mclstr_spd);
    draw_knob(2, encoders[2], mclstr_dep1);
    draw_knob(3, encoders[3], mclstr_dep2);
    panel_info2 = "LFO>MOD";
  }
  else { //if (page_mode == LFO_OFFSET) {
    char mult_label[5];
    lfo_mult_label(encoders[1]->cur, mult_label);
    mcl_gui.draw_knob(0, mclstr_mode, lfo_mode_label(encoders[0]->cur));
    mcl_gui.draw_knob(1, mclstr_mult, mult_label);
    draw_knob(2, encoders[2], mclstr_ofs1);
    draw_knob(3, encoders[3], mclstr_ofs2);
    panel_info2 = "LFO>OFS";
  }

  oled_display.setFont(&TomThumb);

  const uint64_t slide_mask = 0;
  const uint64_t mute_mask = 0;
  uint8_t base_mode = lfo_track->base_mode();
  const char *mode_label = lfo_mode_label(base_mode);

  panel_info1 = mode_label;

  if (base_mode == LFO_MODE_TRIG || base_mode == LFO_MODE_ONE) {
    draw_lock_mask(0, 0, lfo_track->step_count, lfo_track->length, true);
    draw_mask(0, lfo_track->pattern_mask, lfo_track->step_count,
              lfo_track->length, mute_mask, slide_mask);
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
  oled_display.fillRect(MCLGUI::pane_label_x, MCLGUI::pane_label_md_y,
                        MCLGUI::pane_label_w, MCLGUI::pane_label_h * 2,
                        BLACK);
  mcl_gui.draw_panel_toggle("ON", "OFF", lfo_track->enable);
  draw_page_index(false, lfo_track->step_count / 4);

}

void LFOPage::capture_seq_menu_values(bool is_md_device) {
  (void)is_md_device;
  track_update();
  opt_trackid = lfo_track->track_number + 1;
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
  lfo_track->speed = opt_speed;
  lfo_track->mode = (lfo_track->mode & LFO_MODE_LEGACY_FLAGS) |
                    LFOSeqTrack::pack_mode(lfo_track->base_mode(),
                                           opt_lfo_mult);
  lfo_track->phase_inc =
      LFOSeqTrack::speed_to_phase_increment(opt_speed, opt_lfo_mult);
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
  learn_param(DeviceIdx::Primary, track + 1, param, value);
}

void LFOPage::learn_param(DeviceIdx device_idx, uint8_t dest, uint8_t param,
                          uint8_t value) {
  track_update();
  if (lfo_track->device_idx != device_idx) {
    return;
  }
  if (LFOTrackRef::param_count(device_idx, dest) <= param) {
    return;
  }
  bool reconfig = false;
  bool on_lfo_page = mcl.currentPage() == LFO_PAGE;
  if (on_lfo_page && page_mode == LFO_DESTINATION) {
    for (uint8_t i = 0; i < NUM_LFO_PARAMS; ++i) {
      uint8_t encoder_idx = i << 1;
      if (encoders[encoder_idx]->cur == 0 &&
          encoders[encoder_idx + 1]->cur > 0) {
        lfo_track->params[i].dest = dest;
        lfo_track->params[i].param = param;
        reconfig = true;
      }
    }
  }
  for (uint8_t i = 0; i < NUM_LFO_PARAMS; ++i) {
    if (lfo_track->params[i].dest == dest &&
        lfo_track->params[i].param == param) {
      lfo_track->params[i].offset = value;
      reconfig = true;
    }
  }
  if (on_lfo_page && reconfig) { config_encoders(); }
}


bool LFOPage::handleEvent(gui_event_t *event) {
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
    if (!LFOTrackRef::supports_trig_port(port)) {
      return true;
    }

    uint8_t step = event->source;
    if (event->mask == EVENT_BUTTON_PRESSED) {
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
