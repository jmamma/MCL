#include "LFOPage.h"
#include "DeviceParamResolver.h"
#include "LFO.h"
#include "MCLGUI.h"
#include "ResourceManager.h"
#include "SeqPages.h"

#define LFO_TYPE 0
#define LFO_PARAM 1
#define INTERPOLATE
#define DIV_1_127 .0079

#define LFO_MODULATION 2
#define LFO_DESTINATION 1
#define LFO_GLOBAL 0

namespace {

LFOSeqTrackData lfo_clipboard;
bool lfo_clipboard_valid = false;

constexpr uint8_t kLfoMaskPageSteps = 16;

uint8_t lfo_mask_page_count(uint8_t length) {
  if (length == 0) {
    length = kLfoMaskPageSteps;
  }
  if (length > 64) {
    length = 64;
  }
  return (length + kLfoMaskPageSteps - 1) / kLfoMaskPageSteps;
}

uint8_t lfo_mask_page_offset() {
  return SeqPage::page_select * kLfoMaskPageSteps;
}

void clamp_lfo_mask_page(const LFOSeqTrack *track) {
  SeqPage::page_count = lfo_mask_page_count(track->length);
  if (SeqPage::page_select >= SeqPage::page_count) {
    SeqPage::page_select = 0;
  }
}

uint16_t lfo_visible_mask(const LFOSeqTrack *track, uint8_t offset) {
  uint64_t mask = track->pattern_mask;
  uint8_t length = track->length ? track->length : kLfoMaskPageSteps;
  if (length < 64) {
    mask &= (1ULL << length) - 1;
  }
  return (uint16_t)(mask >> offset);
}

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

void refresh_lfo_offset_from_target(LFOSeqTrack *track, uint8_t param_idx) {
  uint8_t value = 0;
  if (LFOTrackRef::get_base_param(track->device_idx,
                                  track->params[param_idx].dest,
                                  track->params[param_idx].param, &value)) {
    track->params[param_idx].offset = value;
  }
}

void update_lfo_param_pair(Encoder **encoders, LFOSeqTrack *track,
                           uint8_t encoder_idx,
                           uint8_t param_idx) NOINLINE();
void update_lfo_param_pair(Encoder **encoders, LFOSeqTrack *track,
                           uint8_t encoder_idx, uint8_t param_idx) {
  MCLEncoder *dest_encoder = (MCLEncoder *)encoders[encoder_idx];
  MCLEncoder *param_encoder = (MCLEncoder *)encoders[encoder_idx + 1];
  dest_encoder->max = LFOTrackRef::target_count(track->device_idx);

  bool changed = false;
  if (dest_encoder->hasChanged()) {
    track->params[param_idx].dest = dest_encoder->cur;
    changed = true;
  }

  uint8_t param_count =
      LFOTrackRef::param_count(track->device_idx, track->params[param_idx].dest);
  param_encoder->max = track->params[param_idx].dest == 0
                           ? 2
                           : (param_count ? param_count - 1 : 0);
  if (param_encoder->cur > param_encoder->max) {
    param_encoder->setValue(param_encoder->max);
    track->params[param_idx].param = param_encoder->cur;
    changed = true;
  } else if (param_encoder->hasChanged()) {
    track->params[param_idx].param = param_encoder->cur;
    changed = true;
  }

  if (changed) {
    refresh_lfo_offset_from_target(track, param_idx);
  }
}

const char lfo_mult_label_1x[] PROGMEM = "1x";
const char lfo_mult_label_2x[] PROGMEM = "2x";
const char lfo_mult_label_4x[] PROGMEM = "4x";
const char lfo_mult_label_8x[] PROGMEM = "8x";
const char lfo_mult_label_half[] PROGMEM = ".5";
const char lfo_mult_label_quarter[] PROGMEM = ".25";
const char lfo_mult_label_tenth[] PROGMEM = ".1";
const char lfo_mult_label_hundredth[] PROGMEM = ".01";
const char *const lfo_mult_labels[LFO_SPEED_MULT_COUNT] PROGMEM = {
    lfo_mult_label_hundredth, lfo_mult_label_tenth,
    lfo_mult_label_quarter,   lfo_mult_label_half,
    lfo_mult_label_1x,        lfo_mult_label_2x,
    lfo_mult_label_4x,        lfo_mult_label_8x};

void lfo_mult_label(uint8_t multiplier, char *out) {
  if (multiplier >= LFO_SPEED_MULT_COUNT) {
    multiplier = LFO_SPEED_MULT_1X;
  }
  PGM_P label = (PGM_P)pgm_read_ptr(&lfo_mult_labels[multiplier]);
  strcpy_P(out, label);
}

void draw_lfo_enable_info_label(bool enabled) {
  const uint8_t split_x = MCLGUI::pane_w / 2;
  oled_display.setFont(&TomThumb);

  oled_display.fillRect(0, MCLGUI::pane_info1_y, MCLGUI::pane_w,
                        MCLGUI::pane_info_h, BLACK);
  oled_display.fillRect(enabled ? 0 : split_x, MCLGUI::pane_info1_y,
                        enabled ? split_x : MCLGUI::pane_w - split_x,
                        MCLGUI::pane_info_h, WHITE);

  oled_display.setTextColor(enabled ? BLACK : WHITE);
  oled_display.setCursor(3, MCLGUI::pane_info1_y + 6);
  oled_display.print("ON");
  oled_display.setTextColor(enabled ? WHITE : BLACK);
  oled_display.setCursor(split_x + 2, MCLGUI::pane_info1_y + 6);
  oled_display.print("OFF");
  oled_display.setTextColor(WHITE);
}

bool lfo_preview_is_centered(uint8_t wav_type) {
  constexpr uint16_t centered_mask = (1 << TRI_WAV) | (1 << SAW_WAV) |
                                     (1 << SQU_WAV) | (1 << RND_WAV) |
                                     (1 << SIN_WAV);
  return (centered_mask >> wav_type) & 1;
}

void update_lfo_base_param(Encoder **encoders, LFOSeqTrack *track,
                           uint8_t encoder_idx, uint8_t param_idx) NOINLINE();
void update_lfo_base_param(Encoder **encoders, LFOSeqTrack *track,
                           uint8_t encoder_idx, uint8_t param_idx) {
  if (!encoders[encoder_idx]->hasChanged()) {
    return;
  }
  uint8_t value = encoders[encoder_idx]->cur;
  track->params[param_idx].offset = value;
  if (track->params[param_idx].dest != 0 &&
      LFOTrackRef::set_base_param(track->device_idx,
                                  track->params[param_idx].dest,
                                  track->params[param_idx].param, value)) {
    track->last_wav_value[param_idx] = value;
  }
}

void lfo_param_label(LFOSeqTrack *track, uint8_t param_idx, char *out,
                     uint8_t len) {
  if (out == nullptr || len == 0) {
    return;
  }
  if (track->params[param_idx].dest == 0) {
    strncpy(out, "---", len - 1);
    out[len - 1] = '\0';
    return;
  }
  if (!LFOTrackRef::param_label(track->device_idx, track->params[param_idx].dest,
                                track->params[param_idx].param, out, len)) {
    mcl_gui.put_value_at(track->params[param_idx].param, out);
  }
}

void draw_lfo_dest(uint8_t knob, uint8_t value, DeviceIdx device_idx) {
  char label[5];
  if (value == 0) {
    strcpy_P(label, mclstr_dash);
  } else if (!LFOTrackRef::target_label(device_idx, value, label,
                                        sizeof(label))) {
    DeviceIdx target_device = DeviceIdx::None;
    uint8_t target = 0;
    if (DeviceParamResolver::perf_dest_to_target(value, &target_device,
                                                 &target)) {
      label[0] = target_device == DeviceIdx::Secondary ? 'M' : 'T';
      mcl_gui.put_value_at(target + 1, label + 1);
    } else {
      mcl_gui.put_value_at(value, label);
    }
  }
  mcl_gui.draw_knob(knob, mclstr_dest, label);
}

void draw_lfo_param(uint8_t knob, uint8_t dest, uint8_t param,
                    DeviceIdx device_idx) {
  char label[4];
  mclstr_copy_progmem(label, mclstr_dash_space, sizeof(label));
  if (dest == 0) {
    if (param > 1) {
      strcpy_P(label, mclstr_ler);
    }
  } else if (!LFOTrackRef::param_label(device_idx, dest, param, label,
                                       sizeof(label))) {
    mcl_gui.put_value_at(param, label);
  }
  mcl_gui.draw_knob(knob, mclstr_par, label);
}

void draw_lfo_value_knob(uint8_t knob, Encoder *encoder, LFOSeqTrack *track,
                         uint8_t param_idx) {
  char label[5];
  lfo_param_label(track, param_idx, label, sizeof(label));
  uint8_t value = encoder->cur;
  uint8_t x = mcl_gui.knob_x0 + knob * mcl_gui.knob_w;
  LightPage *page = GUI.currentPage();
  bool highlight = page != nullptr && page->isEncoderFocused(knob);
  bool show_base = mcl_gui.show_encoder_value(encoder);
  if (highlight && !encoder->hasChanged()) {
    show_base = false;
  }
  if (!show_base) {
    value = track->last_wav_value[param_idx] == 255
                ? track->params[param_idx].offset
                : track->last_wav_value[param_idx];
  }
  mcl_gui.draw_light_encoder(x + 7, 6, value, label, highlight, show_base,
                             false);
}

void draw_lfo_wave_preview(uint8_t knob, uint8_t wav_type) {
  const uint8_t knob_x = mcl_gui.knob_x0 + knob * mcl_gui.knob_w;
  const uint8_t preview_x = knob_x + 5;
  const uint8_t preview_y = 10;
  const uint8_t preview_height = 7;
  const uint8_t preview_width = 15;

  oled_display.setFont(&TomThumb);
  oled_display.setTextColor(WHITE);
  oled_display.setCursor(knob_x + 5, mcl_gui.knob_y0 + 6);
  mcl_print_P(mclstr_wav_label);
  oled_display.setFont();

  wav_type = wav_type < LFO_SHAPE_COUNT ? wav_type : TRI_WAV;
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

  LightPage *page = GUI.currentPage();
  if (page != nullptr && page->isEncoderFocused(knob)) {
    oled_display.fillRect(knob_x, 0, mcl_gui.knob_w,
                          preview_y + preview_height + 1, INVERT);
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

bool LFOPage::refresh_track_selection() {
  LFOSeqTrack *old_track = lfo_track;
  track_update();
  if (lfo_track == old_track) {
    return false;
  }
  sync_lfo_track();
  update_lfo_key_interface(lfo_track);
  clamp_lfo_mask_page(lfo_track);
  config_encoders();
  return true;
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
  display_page_index = false;
  SeqPage::init();
  track_update();
  clamp_lfo_mask_page(lfo_track);

  constexpr uint32_t lfo_menu_entries =
      menu_entry_mask(SEQ_MENU_TRACK) | menu_entry_mask(SEQ_MENU_DEVICE) |
      menu_entry_mask(SEQ_MENU_SPEED) | menu_entry_mask(SEQ_MENU_LENGTH_MD);
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
  ((MCLEncoder *)encoders[i + 1])->max = encoders[i]->cur == 0
                                             ? 2
                                             : (param_count ? param_count - 1
                                                            : 0);
}

void LFOPage::config_encoders() {
  if (show_seq_menu) {
    return;
  }
  track_update();
  if (page_mode == LFO_DESTINATION) {
    encoders[0]->cur = lfo_track->params[0].dest;
    encoders[1]->cur = lfo_track->params[0].param;
    encoders[2]->cur = lfo_track->params[1].dest;
    encoders[3]->cur = lfo_track->params[1].param;

    config_encoder_range(0);
    config_encoder_range(2);
  }
  else if (page_mode == LFO_GLOBAL) {
    encoders[0]->cur = lfo_track->base_mode();
    ((MCLEncoder *)encoders[0])->max = LFO_MODE_TRACK_TRIG;

    encoders[1]->cur = lfo_track->wav_type;
    ((MCLEncoder *)encoders[1])->max = LFO_WAV_COUNT - 1;

    encoders[2]->cur = lfo_track->speed;
    ((MCLEncoder *)encoders[2])->max = 127;

    encoders[3]->cur = lfo_track->speed_multiplier();
    ((MCLEncoder *)encoders[3])->max = LFO_SPEED_MULT_COUNT - 1;
  }
  else if (page_mode == LFO_MODULATION) {
    encoders[0]->cur = lfo_track->params[0].depth;
    ((MCLEncoder *)encoders[0])->max = 127;

    encoders[1]->cur = lfo_track->params[0].offset;
    ((MCLEncoder *)encoders[1])->max = 127;

    encoders[2]->cur = lfo_track->params[1].depth;
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
    page_mode = page_mode < LFO_MODULATION ? page_mode + 1 : 0;
  } else {
    page_mode = page_mode > 0 ? page_mode - 1 : LFO_MODULATION;
  }
  config_encoders();
  return true;
}

void LFOPage::loop() {
  refresh_track_selection();
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
    update_lfo_param_pair(encoders, lfo_track, 0, 0);
    update_lfo_param_pair(encoders, lfo_track, 2, 1);
  }
  else if (page_mode == LFO_GLOBAL) {
    if (encoders[0]->hasChanged()) {
      lfo_track->set_mode(encoders[0]->cur);
      update_lfo_key_interface(lfo_track);
    }

    if (encoders[1]->hasChanged()) {
      lfo_track->set_wav_type(encoders[1]->cur);
    }

    if (encoders[2]->hasChanged()) {
      lfo_track->set_speed(encoders[2]->cur);
    }

    if (encoders[3]->hasChanged()) {
      lfo_track->set_speed_multiplier(encoders[3]->cur);
    }
  }

  else if (page_mode == LFO_MODULATION) {
    if (encoders[0]->hasChanged()) {
      lfo_track->set_depth(0, encoders[0]->cur);
    }
    if (encoders[2]->hasChanged()) {
      lfo_track->set_depth(1, encoders[2]->cur);
    }

    update_lfo_base_param(encoders, lfo_track, 1, 0);
    update_lfo_base_param(encoders, lfo_track, 3, 1);
  }

}

void LFOPage::display() {
  refresh_track_selection();
  oled_display.clearDisplay();

  // mcl_gui.draw_vertical_dashline(x, 0, knob_y);
  SeqPage::draw_knob_frame();
  if (page_mode == LFO_DESTINATION) {
    DeviceIdx device_idx = lfo_track->device_idx;
    draw_lfo_dest(0, encoders[0]->cur, device_idx);
    draw_lfo_param(1, encoders[0]->cur, encoders[1]->cur, device_idx);
    draw_lfo_dest(2, encoders[2]->cur, device_idx);
    draw_lfo_param(3, encoders[2]->cur, encoders[3]->cur, device_idx);
    strcpy(info2, "LFO>DST");
  }
  else if (page_mode == LFO_GLOBAL) {
    char mult_label[5];
    lfo_mult_label(encoders[3]->cur, mult_label);
    mcl_gui.draw_knob(0, mclstr_mode, lfo_mode_label(encoders[0]->cur));
    draw_lfo_wave_preview(1, encoders[1]->cur);
    draw_knob(2, encoders[2], mclstr_spd);
    mcl_gui.draw_knob(3, mclstr_mult, mult_label);
    strcpy(info2, "LFO>SET");
  }
  else {
    draw_knob(0, encoders[0], mclstr_dep1);
    draw_lfo_value_knob(1, encoders[1], lfo_track, 0);
    draw_knob(2, encoders[2], mclstr_dep2);
    draw_lfo_value_knob(3, encoders[3], lfo_track, 1);
    strcpy(info2, "LFO>DEP");
  }

  oled_display.setFont(&TomThumb);

  uint8_t base_mode = lfo_track->base_mode();
  bool show_lfo_mask = base_mode == LFO_MODE_TRIG || base_mode == LFO_MODE_ONE;

  if (show_lfo_mask) {
    const uint64_t empty_mask = 0;
    clamp_lfo_mask_page(lfo_track);
    uint8_t offset = lfo_mask_page_offset();
    draw_lock_mask(offset, empty_mask, lfo_track->step_count, lfo_track->length,
                   true);
    draw_mask(offset, lfo_track->pattern_mask, lfo_track->step_count,
              lfo_track->length, empty_mask, empty_mask);
    uint16_t visible_mask = lfo_visible_mask(lfo_track, offset);
    if (visible_mask != trigled_mask) {
      trigled_mask = visible_mask;
      mcl_gui.set_trigleds(visible_mask, TRIGLED_STEPEDIT);
    }
  }

  info1[0] = '\0';
  SeqPage::display();
  draw_lfo_enable_info_label(lfo_track->enable);
  if (show_lfo_mask) {
    draw_page_index(true, lfo_track->step_count / kLfoMaskPageSteps);
  }

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
  lfo_track->set_speed(opt_speed);
  if (opt_length == 0) {
    opt_length = 1;
  }
  if (opt_length > 64) {
    opt_length = 64;
  }
  lfo_track->length = opt_length;
  if (lfo_track->step_count >= lfo_track->length) {
    lfo_track->step_count = 0;
  }
  clamp_lfo_mask_page(lfo_track);
  sync_lfo_track();
}

bool LFOPage::apply_seq_menu_row(uint8_t row_entry, void (*row_func)()) {
  switch (row_entry) {
  case SEQ_MENU_TRACK:
    select_menu_track(opt_trackid ? opt_trackid - 1 : 0);
    return true;
  case SEQ_MENU_DEVICE:
  case SEQ_MENU_SPEED:
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
  if (dest == 0) {
    return;
  }
  uint8_t global_dest =
      DeviceParamResolver::perf_dest_for_target(device_idx, dest - 1);
  if (global_dest == 0 ||
      LFOTrackRef::param_count(lfo_track->device_idx, global_dest) <= param) {
    return;
  }
  bool reconfig = false;
  bool on_lfo_page = mcl.currentPage() == LFO_PAGE;
  if (on_lfo_page && page_mode == LFO_DESTINATION) {
    for (uint8_t i = 0; i < NUM_LFO_PARAMS; ++i) {
      uint8_t encoder_idx = i << 1;
      if (encoders[encoder_idx]->cur == 0 &&
          encoders[encoder_idx + 1]->cur > 0) {
        lfo_track->params[i].dest = global_dest;
        lfo_track->params[i].param = param;
        reconfig = true;
      }
    }
  }
  for (uint8_t i = 0; i < NUM_LFO_PARAMS; ++i) {
    if (lfo_track->params[i].dest == global_dest &&
        lfo_track->params[i].param == param) {
      lfo_track->params[i].offset = value;
      reconfig = true;
    }
  }
  if (on_lfo_page && reconfig) { config_encoders(); }
}

void LFOPage::clear_lfo_track() {
  DeviceIdx device_idx = lfo_track->device_idx;
  uint8_t track_number = lfo_track->track_number;
  if (lfo_track->enable) {
    lfo_track->reset_params();
  }
  lfo_track->LFOSeqTrackData::init();
  lfo_track->device_idx = device_idx;
  lfo_track->track_number = track_number;
  for (uint8_t i = 0; i < NUM_LFO_PARAMS; ++i) {
    lfo_track->last_wav_value[i] = 255;
  }
  lfo_track->reset_runtime();
  clamp_lfo_mask_page(lfo_track);
  sync_lfo_track();
  update_lfo_key_interface(lfo_track);
  config_encoders();
  oled_display.textbox_P(mclstr_clear, mclstr_track);
}

void LFOPage::copy_lfo_track() {
  lfo_clipboard = *lfo_track;
  lfo_clipboard_valid = true;
  oled_display.textbox_P(mclstr_copy, mclstr_track);
}

void LFOPage::paste_lfo_track() {
  if (!lfo_clipboard_valid) {
    return;
  }
  if (lfo_track->enable) {
    lfo_track->reset_params();
  }
  lfo_track->LFOSeqTrackData::operator=(lfo_clipboard);
  for (uint8_t i = 0; i < NUM_LFO_PARAMS; ++i) {
    lfo_track->last_wav_value[i] = 255;
  }
  lfo_track->reset_runtime();
  clamp_lfo_mask_page(lfo_track);
  sync_lfo_track();
  update_lfo_key_interface(lfo_track);
  config_encoders();
  oled_display.textbox_P(mclstr_paste, mclstr_track);
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

    uint8_t length = lfo_track->length ? lfo_track->length
                                       : kLfoMaskPageSteps;
    uint8_t step = event->source + lfo_mask_page_offset();
    if (step >= length) {
      return true;
    }
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
      if (key_interface.is_key_down(MDX_KEY_FUNC)) {
        switch (key) {
        case MDX_KEY_CLEAR:
          clear_lfo_track();
          return true;
        case MDX_KEY_COPY:
          copy_lfo_track();
          return true;
        case MDX_KEY_PASTE:
          paste_lfo_track();
          return true;
        default:
          break;
        }
      }
      switch (key) {
      case MDX_KEY_YES: {
        goto lfo_enable;
      }
      }
    } else if (event->mask == EVENT_BUTTON_RELEASED && key == MDX_KEY_SCALE) {
      page_select++;
      clamp_lfo_mask_page(lfo_track);
      trigled_mask = 0xFFFF;
      return true;
    }
  }
  if (EVENT_BUTTON(event)) {
    if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
      page_mode++;
      if (page_mode > LFO_MODULATION) { page_mode = 0; }
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
