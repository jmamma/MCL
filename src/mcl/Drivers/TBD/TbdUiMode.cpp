#include "TbdUiMode.h"

#ifdef PLATFORM_TBD

#include "MCL.h"
#include "MCLGUI.h"
#include "MCLSeq.h"
#include "MidiClock.h"
#include "MidiSetup.h"
#include "NoteInterface.h"
#include "SeqExtStepTrackApi.h"
#include "SeqPages.h"
#include "SeqStepTrackApi.h"
#include "TBD.h"
#include "TbdP4Realtime.h"
#include "GUI_hardware.h"
#include <string.h>

TbdUiMode tbd_ui_mode;
TbdParamStripPage tbd_param_strip_page;
TbdParamOverlayPage tbd_param_overlay_page;

namespace {

uint8_t normalized_value(const TbdP4ParamDescriptor &param) {
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
}

uint16_t value14_from_normalized(uint8_t value) {
  if (value > 127) value = 127;
  return (uint16_t)(((uint32_t)value * 0x3FFFu + 63u) / 127u);
}

bool is_tbd_param_visible(const TbdP4ParamDescriptor *param) {
  return param != nullptr && param->is_visible();
}

static void invert_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
  for (uint8_t yy = y; yy < y + h; yy++) {
    for (uint8_t xx = x; xx < x + w; xx++) {
      oled_display.drawPixel(xx, yy, 2);
    }
  }
}

TbdP4ParamDescriptor *mutable_param_for_lock(TbdP4SoundData &sound,
                                             uint8_t lock_param) {
  if (lock_param < TBD_P4_AUDIO_PARAM_COUNT) {
    return &sound.audio_params.params[lock_param];
  }
  if (lock_param >= TBD_P4_LOCK_MIXER_PARAM_BASE &&
      lock_param < TBD_P4_LOCK_RESERVED_PARAM_BASE) {
    return &sound.mixer_params.params[lock_param - TBD_P4_LOCK_MIXER_PARAM_BASE];
  }
  return nullptr;
}

uint8_t clamped_audio_pages(const TbdP4SoundData &sound) {
  return min(sound.audio_params.num_pages,
             (uint8_t)TBD_P4_AUDIO_PARAM_PAGE_COUNT);
}

uint8_t clamped_mixer_pages(const TbdP4SoundData &sound) {
  return min(sound.mixer_params.num_pages,
             (uint8_t)TBD_P4_MIXER_PARAM_PAGE_COUNT);
}

void copy_text(const char *src, char *dst, size_t dst_len,
               uint8_t max_chars) {
  if (dst == nullptr || dst_len == 0) return;
  dst[0] = '\0';
  if (src == nullptr) return;

  size_t out = 0;
  bool last_space = false;
  while (*src && out + 1 < dst_len && out < max_chars) {
    char c = *src++;
    if (c == '_' || c == ' ') {
      if (out == 0 || last_space) continue;
      c = ' ';
      last_space = true;
    } else {
      last_space = false;
    }
    if (c >= 'a' && c <= 'z') {
      c = (char)(c - ('a' - 'A'));
    }
    dst[out++] = c;
  }
  dst[out] = '\0';
}

void copy_fixed(const char *src, char *dst, size_t dst_len) {
  if (dst == nullptr || dst_len == 0) return;
  size_t out = 0;
  while (src && src[out] && out + 1 < dst_len) {
    dst[out] = src[out];
    out++;
  }
  dst[out] = '\0';
}

void put_uint8_2(uint8_t value, char *dst) {
  if (value > 99) value = 99;
  dst[0] = (char)('0' + value / 10);
  dst[1] = (char)('0' + value % 10);
}

void put_int16_cell(int16_t value, char *dst, size_t dst_len) {
  if (dst == nullptr || dst_len == 0) return;
  if (value > 9999) value = 9999;
  if (value < -9999) value = -9999;

  bool negative = value < 0;
  uint16_t mag = negative ? (uint16_t)(-value) : (uint16_t)value;
  char rev[5];
  uint8_t digits = 0;
  do {
    rev[digits++] = (char)('0' + (mag % 10));
    mag /= 10;
  } while (mag && digits < sizeof(rev));

  size_t out = 0;
  if (negative && out + 1 < dst_len) {
    dst[out++] = '-';
  }
  while (digits && out + 1 < dst_len) {
    dst[out++] = rev[--digits];
  }
  dst[out] = '\0';
}

void draw_text_limited(uint8_t x, uint8_t y, const char *text,
                       uint8_t max_chars) {
  oled_display.setCursor(x, y);
  for (uint8_t i = 0; text && text[i] && i < max_chars; i++) {
    oled_display.write(text[i]);
  }
}

void draw_text_centered(uint8_t x, uint8_t y, uint8_t width,
                        const char *text, uint8_t max_chars) {
  uint8_t len = 0;
  while (text && text[len] && len < max_chars) len++;
  uint8_t text_w = len * 6;
  uint8_t text_x = x;
  if (text_w < width) {
    text_x = x + (width - text_w) / 2;
  }
  draw_text_limited(text_x, y, text, max_chars);
}

void copy_window_label(uint8_t window, char *dst, size_t dst_len) {
  if (dst == nullptr || dst_len == 0) return;
  dst[0] = '\0';

  TbdP4SoundData *sound = tbd_ui_mode.active_sound();
  if (sound == nullptr) {
    copy_fixed("P4", dst, dst_len);
    return;
  }

  const uint8_t audio_pages = clamped_audio_pages(*sound);
  if (window < audio_pages) {
    copy_text(sound->audio_params.pages[window].name, dst, dst_len, 10);
    if (dst[0] == '\0') {
      dst[0] = 'A';
      dst[1] = (char)('1' + window);
      dst[2] = '\0';
    }
    return;
  }

  window -= audio_pages;
  if (window < clamped_mixer_pages(*sound)) {
    copy_text(sound->mixer_params.pages[window].name, dst, dst_len, 10);
    if (dst[0] == '\0') {
      copy_fixed("MIX", dst, dst_len);
      if (dst_len > 4) {
        dst[3] = (char)('1' + window);
        dst[4] = '\0';
      }
    }
    return;
  }

  copy_fixed("P4", dst, dst_len);
}

void copy_sound_label(char *dst, size_t dst_len) {
  if (dst == nullptr || dst_len == 0) return;
  dst[0] = '\0';
  TbdP4SoundData *sound = tbd_ui_mode.active_sound();
  if (sound == nullptr) {
    copy_fixed("P4", dst, dst_len);
    return;
  }
  copy_text(sound->machine_id, dst, dst_len, 8);
  if (dst[0] == '\0') copy_text(sound->preset_name, dst, dst_len, 8);
  if (dst[0] == '\0') copy_text(sound->preset_id, dst, dst_len, 8);
  if (dst[0] == '\0') copy_fixed("P4", dst, dst_len);
}

void copy_track_label(char *dst, size_t dst_len) {
  if (dst == nullptr || dst_len < 9) return;
  dst[0] = 'T';
  dst[1] = 'B';
  dst[2] = 'D';
  dst[3] = tbd_ui_mode.device_idx() == TbdUiMode::SLOT_SECONDARY ? '2' : '1';
  dst[4] = ' ';
  dst[5] = 'T';
  put_uint8_2(tbd_ui_mode.active_track_index() + 1, dst + 6);
  dst[8] = '\0';
}

bool active_track_muted() {
  if (tbd_ui_mode.device_idx() == TbdUiMode::SLOT_PRIMARY) {
    if (mcl_cfg.grid_x_device != GRID_X_DEVICE_TBD ||
        last_md_track >= mcl_seq.num_tbd_tracks) {
      return false;
    }
    return seq_step_api_active_track(true).mute_state() != 0;
  }
#ifdef EXT_TRACKS
  if (tbd_ui_mode.device_idx() == TbdUiMode::SLOT_SECONDARY &&
      last_ext_track < NUM_EXT_TRACKS) {
    return mcl_seq.midi_tracks[last_ext_track].mute_state != 0;
  }
#endif
  return false;
}

void copy_status_flags(char *dst, size_t dst_len) {
  if (dst == nullptr || dst_len < 4) return;
  dst[0] = SeqPage::recording ? 'R' : '-';
  dst[1] = MidiClock.state == MidiClockClass::STARTED ? 'P' : '-';
  dst[2] = active_track_muted() ? 'M' : '-';
  dst[3] = '\0';
}

void format_param_value(const TbdP4ParamDescriptor &param, uint8_t value,
                        char *dst, size_t dst_len) {
  put_int16_cell(tbd_p4_scale_lock_value(param, value14_from_normalized(value)),
                 dst, dst_len);
}

void draw_overlay_header() {
  oled_display.fillRect(0, 0, 128, 8, BLACK);
  auto oldfont = oled_display.getFont();
  oled_display.setFont();
  oled_display.setTextColor(WHITE, BLACK);

  char track[9];
  char sound[9];
  char flags[4];
  copy_track_label(track, sizeof(track));
  copy_sound_label(sound, sizeof(sound));
  copy_status_flags(flags, sizeof(flags));

  draw_text_limited(0, 0, track, 8);
  draw_text_centered(50, 0, 48, sound, 8);
  draw_text_limited(110, 0, flags, 3);
  oled_display.setFont(oldfont);
}

void render_window(uint8_t y_top, uint8_t window, bool active,
                   uint8_t row_height) {
  Encoder proxies[4];
  TbdUiMode::ParamSlot slots[4];
  bool has_slot[4] = {};
  bool locked[4] = {};
  uint8_t display_value[4] = {};

  oled_display.fillRect(0, y_top, 128, row_height, BLACK);
  auto oldfont = oled_display.getFont();
  oled_display.setFont();
  oled_display.setTextColor(WHITE, BLACK);

  char page_label[12];
  copy_window_label(window, page_label, sizeof(page_label));
  draw_text_limited(2, y_top, page_label, 10);

  for (uint8_t i = 0; i < 4; i++) {
    if (!tbd_ui_mode.param_slot(window, i, slots[i]) ||
        !is_tbd_param_visible(slots[i].param)) {
      continue;
    }

    has_slot[i] = true;
    uint8_t lock_value = 0;
    if (tbd_ui_mode.active_step_lock(window, i, &lock_value)) {
      proxies[i].setValue(lock_value);
      locked[i] = true;
      display_value[i] = lock_value;
    } else if (active) {
      display_value[i] = (uint8_t)tbd_ui_mode.enc[i].cur;
    } else {
      proxies[i].setValue(normalized_value(*slots[i].param));
      display_value[i] = (uint8_t)proxies[i].cur;
    }
  }

  constexpr uint8_t kCellW = 32;
  constexpr uint8_t kDialW = 11;
  const uint8_t dial_y = y_top + 8;
  const uint8_t text_y = y_top + (row_height == 28 ? 20 : 22);
  for (uint8_t i = 0; i < 4; i++) {
    if (!has_slot[i]) continue;
    uint8_t cx = i * kCellW;
    Encoder *draw_enc = &proxies[i];
    if (locked[i]) {
      draw_enc = &proxies[i];
    } else if (active) {
      draw_enc = &tbd_ui_mode.enc[i];
    }
    mcl_gui.draw_encoder(cx + (kCellW - kDialW) / 2, dial_y, draw_enc);

    char text[8];
    bool show_value = locked[i] ||
                      (active && tbd_ui_mode.show_strip_value(i));
    if (show_value) {
      format_param_value(*slots[i].param, display_value[i], text, sizeof(text));
    } else {
      copy_text(slots[i].param->shortname, text, sizeof(text), 5);
      if (text[0] == '\0') {
        text[0] = slots[i].param->ctrl_type == TBD_P4_CTRLTYPE_NRPM ? 'N' : 'P';
        text[1] = (char)('0' + (slots[i].lock_param / 10) % 10);
        text[2] = (char)('0' + slots[i].lock_param % 10);
        text[3] = '\0';
      }
    }
    draw_text_centered(cx, text_y, kCellW, text, 5);

    if (locked[i]) {
      uint8_t text_w = (uint8_t)strlen(text) * 6;
      uint8_t inner_w = (text_w > kDialW) ? text_w : kDialW;
      uint8_t inv_x = cx + (kCellW - inner_w) / 2 - 2;
      invert_rect(inv_x, dial_y - 1, inner_w + 4,
                  (uint8_t)(text_y + 8 - (dial_y - 1)));
    }
  }

  oled_display.setFont(oldfont);
}

} // namespace

uint8_t TbdUiMode::active_track_index() const {
  return device_idx_ == SLOT_SECONDARY ? last_ext_track : last_md_track;
}

TbdP4SoundData *TbdUiMode::active_sound() const {
  if (!latched_) return nullptr;
  if (device_idx_ == SLOT_PRIMARY) {
    if (mcl_cfg.grid_x_device != GRID_X_DEVICE_TBD) return nullptr;
    if (last_md_track >= mcl_seq.num_tbd_tracks) return nullptr;
    return &mcl_seq.tbd_tracks[last_md_track].p4_sound;
  }
  if (device_idx_ == SLOT_SECONDARY) {
    if (mcl_cfg.grid_y_device != GRID_Y_DEVICE_TBD) return nullptr;
    if (last_ext_track >= NUM_EXT_TRACKS) return nullptr;
    return &mcl_seq.midi_tracks[last_ext_track].p4_sound;
  }
  return nullptr;
}

uint8_t TbdUiMode::window_count() const {
  TbdP4SoundData *sound = active_sound();
  if (sound == nullptr) return 1;

  uint8_t count = clamped_audio_pages(*sound) + clamped_mixer_pages(*sound);
  return count == 0 ? 1 : count;
}

bool TbdUiMode::param_slot(uint8_t window, uint8_t encoder_idx,
                           ParamSlot &slot) const {
  slot = ParamSlot();
  if (encoder_idx >= 4) return false;

  TbdP4SoundData *sound = active_sound();
  if (sound == nullptr) return false;

  const uint8_t audio_pages = clamped_audio_pages(*sound);
  if (window < audio_pages) {
    uint8_t param_idx = window * 4 + encoder_idx;
    if (param_idx >= TBD_P4_AUDIO_PARAM_COUNT) return false;
    slot.sound = sound;
    slot.lock_param = param_idx;
    slot.param = mutable_param_for_lock(*sound, slot.lock_param);
    return is_tbd_param_visible(slot.param);
  }

  window -= audio_pages;
  if (window < clamped_mixer_pages(*sound)) {
    uint8_t mixer_idx = window * 4 + encoder_idx;
    if (mixer_idx >= TBD_P4_MIXER_PARAM_COUNT) return false;
    slot.sound = sound;
    slot.lock_param = TBD_P4_LOCK_MIXER_PARAM_BASE + mixer_idx;
    slot.param = mutable_param_for_lock(*sound, slot.lock_param);
    return is_tbd_param_visible(slot.param);
  }

  return false;
}

bool TbdUiMode::active_step_lock(uint8_t window, uint8_t encoder_idx,
                                 uint8_t *value) const {
  if (value == nullptr || note_interface.notes_count_on() == 0) {
    return false;
  }

  ParamSlot slot;
  if (!param_slot(window, encoder_idx, slot) || slot.param == nullptr) {
    return false;
  }

  if (device_idx_ == SLOT_PRIMARY) {
    if (mcl.currentPage() != SEQ_STEP_PAGE) return false;
    SeqStepTrackApi track = seq_step_api_active_track(true);
    int8_t lock_idx = track.find_param(slot.lock_param);
    if (lock_idx < 0) return false;

    for (uint8_t i = 0; i < 16; i++) {
      if (!note_interface.is_note_on(i)) continue;
      uint8_t step = i + (SeqPage::page_select * 16);
      if (step >= track.length()) return false;
      if (!track.step_has_lock(step, (uint8_t)lock_idx)) return false;
      *value = track.get_track_lock_implicit(step, slot.lock_param);
      return true;
    }
    return false;
  }

#ifdef EXT_TRACKS
  if (device_idx_ == SLOT_SECONDARY) {
    if (mcl.currentPage() != SEQ_EXTSTEP_PAGE ||
        mcl_cfg.grid_y_device != GRID_Y_DEVICE_TBD ||
        last_ext_track >= NUM_EXT_TRACKS) {
      return false;
    }

    SeqExtStepTrackApi track(mcl_seq.midi_tracks[last_ext_track]);
    uint16_t timing_mid = track.ticks_per_step();
    if (timing_mid == 0) return false;
    uint16_t page_width = 16 * timing_mid;
    if (page_width == 0) return false;

    for (uint8_t i = 0; i < 16; i++) {
      if (!note_interface.is_note_on(i)) continue;
      uint8_t step = ((seq_extstep_page.cur_x / page_width) * 16) + i;
      if (step >= track.length()) return false;
      return track.p4_param_lock_value(step, slot.lock_param, *value);
    }
  }
#endif

  return false;
}

bool TbdUiMode::enter(uint8_t device_idx) {
  if (device_idx != SLOT_PRIMARY && device_idx != SLOT_SECONDARY) {
    return false;
  }

  if (latched_ && device_idx_ == device_idx) {
    disable();
    return true;
  }

  latched_ = true;
  device_idx_ = device_idx;
  sub_page_ = min(sub_page_, (uint8_t)(window_count() - 1));
  resync_from_sound();
  show_fullscreen();
  return true;
}

void TbdUiMode::disable() {
  latched_ = false;
  device_idx_ = SLOT_NONE;
  bound_device_idx_ = SLOT_NONE;
  bound_track_ = 255;
  bound_sub_page_ = 255;
  if (GUI.overlay == &tbd_param_strip_page ||
      GUI.overlay == &tbd_param_overlay_page) {
    GUI.clearOverlay();
  }
}

void TbdUiMode::show_fullscreen() {
  GUI.setOverlay(&tbd_param_overlay_page);
}

void TbdUiMode::show_strip() {
  GUI.setOverlay(&tbd_param_strip_page);
}

void TbdUiMode::move_sub_page(int8_t delta) {
  uint8_t count = window_count();
  if (count == 0) return;
  int16_t next = (int16_t)sub_page_ + delta;
  if (next < 0) next = 0;
  if (next >= count) next = count - 1;
  if (sub_page_ != (uint8_t)next) {
    sub_page_ = (uint8_t)next;
    resync_from_sound();
  }
}

void TbdUiMode::flip_sub_page_half() {
  uint8_t count = window_count();
  if (count <= 1) return;
  uint8_t next = sub_page_ ^ 1;
  if ((sub_page_ & 0xFE) != (next & 0xFE) || next >= count) {
    return;
  }
  sub_page_ = next;
  resync_from_sound();
}

bool TbdUiMode::handle_event(gui_event_t *event) {
  if (!latched_ || !EVENT_BUTTON(event)) return false;
  if (event->mask != EVENT_BUTTON_PRESSED &&
      event->mask != EVENT_BUTTON_RELEASED) {
    return false;
  }

  const bool is_press = event->mask == EVENT_BUTTON_PRESSED;
  const bool fullscreen = GUI.overlay == &tbd_param_overlay_page;

  if (event->source == ButtonsClass::FUNC_BUTTON6 ||
      event->source == ButtonsClass::FUNC_BUTTON8) {
    if (is_press && fullscreen) {
      flip_sub_page_half();
    }
    return fullscreen;
  }

  if (event->source == ButtonsClass::FUNC_BUTTON7 ||
      event->source == ButtonsClass::FUNC_BUTTON9) {
    if (is_press) {
      int8_t delta = event->source == ButtonsClass::FUNC_BUTTON7 ? -2 : 2;
      if (!fullscreen && BUTTON_DOWN(ButtonsClass::FUNC_BUTTON5)) {
        delta = event->source == ButtonsClass::FUNC_BUTTON7 ? -1 : 1;
      }
      if (fullscreen || BUTTON_DOWN(ButtonsClass::FUNC_BUTTON5)) {
        move_sub_page(delta);
        return true;
      }
    }
    return fullscreen;
  }

  return false;
}

bool TbdUiMode::encoder_passthrough_page() const {
  PageIndex pg = mcl.currentPage();
  return pg == PAGE_SELECT_PAGE || pg == BANK_POPUP_PAGE;
}

void TbdUiMode::resync_from_sound() {
  TbdP4SoundData *sound = active_sound();
  for (uint8_t i = 0; i < 4; i++) {
    ParamSlot slot;
    if (sound != nullptr && param_slot(sub_page_, i, slot) &&
        is_tbd_param_visible(slot.param)) {
      enc[i].setValue(normalized_value(*slot.param));
    } else {
      enc[i].setValue(0);
    }
  }
  bound_device_idx_ = device_idx_;
  bound_track_ = active_track_index();
  bound_sub_page_ = sub_page_;
}

void TbdUiMode::send_param(uint8_t encoder_idx) {
  ParamSlot slot;
  if (!param_slot(sub_page_, encoder_idx, slot) ||
      !slot.sound || !slot.param || !slot.param->is_sendable()) {
    return;
  }

  uint8_t value = (uint8_t)enc[encoder_idx].cur;
  uint16_t value14 = value14_from_normalized(value);
  int16_t scaled = tbd_p4_scale_lock_value(*slot.param, value14);
  slot.param->value = scaled;

  if (write_step_locks(slot, value)) {
    return;
  }

  if (TBD.uart == nullptr) return;

  tbd_p4_realtime.set_active_track(slot.sound->p4_track_index);
  if (slot.param->ctrl_type == TBD_P4_CTRLTYPE_CC) {
    tbd_p4_send_param_value(TBD.uart, slot.sound->midi_channel, *slot.param,
                            scaled);
  } else if (slot.param->ctrl_type == TBD_P4_CTRLTYPE_NRPM) {
    tbd_p4_send_param_value(TBD.uart, slot.sound->midi_channel, *slot.param,
                            scaled);
  }
}

bool TbdUiMode::write_step_locks(const ParamSlot &slot, uint8_t value) {
  if (slot.param == nullptr || !slot.param->is_sendable()) {
    return false;
  }

  if (device_idx_ == SLOT_PRIMARY) {
    SeqStepTrackApi track = seq_step_api_active_track(true);
    bool wrote = false;
    if (mcl.currentPage() == SEQ_STEP_PAGE &&
        note_interface.notes_count_on() > 0) {
      for (uint8_t n = 0; n < 16; n++) {
        if (!note_interface.is_note_on(n)) continue;
        uint8_t step = n + (SeqPage::page_select * 16);
        if (step >= track.length()) continue;
        if (track.set_track_locks(step, slot.lock_param, value)) {
          track.enable_step_locks(step);
          if (SeqPage::mask_type == MASK_PATTERN) {
            bool cond_plock = false;
            uint8_t condition =
                track.step_conditional_from_knob(seq_param1.cur, &cond_plock);
            track.set_step(step, MASK_PATTERN, true);
            track.set_conditional(step, condition, cond_plock);
            track.set_timing_from_encoder(step, seq_param2.cur);
          }
          wrote = true;
        }
      }
      return wrote;
    }

    if (SeqPage::recording && MidiClock.state == 2) {
      track.record_track_locks(slot.lock_param, value);
      return true;
    }
    return false;
  }

#ifdef EXT_TRACKS
  if (device_idx_ == SLOT_SECONDARY) {
    if (mcl_cfg.grid_y_device != GRID_Y_DEVICE_TBD ||
        last_ext_track >= NUM_EXT_TRACKS) {
      return false;
    }

    SeqExtStepTrackApi track(mcl_seq.midi_tracks[last_ext_track]);
    uint16_t timing_mid = track.ticks_per_step();
    if (timing_mid == 0) return false;

    bool wrote = false;
    if (mcl.currentPage() == SEQ_EXTSTEP_PAGE &&
        note_interface.notes_count_on() > 0) {
      uint16_t page_width = 16 * timing_mid;
      if (page_width == 0) return false;
      for (uint8_t n = 0; n < 16; n++) {
        if (!note_interface.is_note_on(n)) continue;
        uint8_t step = ((seq_extstep_page.cur_x / page_width) * 16) + n;
        if (step >= track.length()) continue;
        if (track.set_p4_param_lock(step, timing_mid, slot.lock_param,
                                    value, SeqPage::slide)) {
          wrote = true;
        }
      }
      return wrote;
    }

    if (SeqPage::recording && MidiClock.state == 2) {
      return track.record_p4_param_lock(slot.lock_param, value, SeqPage::slide);
    }
  }
#endif

  return false;
}

void TbdUiMode::poll_encoders() {
  if (!latched_ || encoder_passthrough_page()) return;

  if (bound_device_idx_ != device_idx_ ||
      bound_track_ != active_track_index() ||
      bound_sub_page_ != sub_page_) {
    resync_from_sound();
  }

  encoder_t snapshot[4];
  for (uint8_t i = 0; i < 4; i++) {
    snapshot[i] = Encoders.encoders[i];
    Encoders.encoders[i].normal = 0;
  }

  uint16_t now = read_clock_ms();
  bool edited = false;
  for (uint8_t i = 0; i < 4; i++) {
    ParamSlot slot;
    if (!param_slot(sub_page_, i, slot)) continue;

    enc[i].update(&snapshot[i]);
    if (enc[i].hasChanged()) {
      send_param(i);
      enc[i].old = enc[i].cur;
      enc_used_clock_[i] = now ? now : 1;
      edited = true;
    }
  }

  if (edited && GUI.overlay == &tbd_param_overlay_page) {
    show_strip();
  }
}

bool TbdUiMode::show_strip_value(uint8_t encoder_idx) const {
  if (encoder_idx >= 4 || enc_used_clock_[encoder_idx] == 0) return false;
  return clock_diff(enc_used_clock_[encoder_idx], read_clock_ms()) <
         SHOW_VALUE_TIMEOUT;
}

void TbdParamStripPage::display() {
  if (!tbd_ui_mode.is_active()) return;
  render_window(32, tbd_ui_mode.sub_page(), true, 32);
}

void TbdParamOverlayPage::display() {
  if (!tbd_ui_mode.is_active()) return;

  uint8_t sub_page = tbd_ui_mode.sub_page();
  uint8_t first = sub_page & 0xFE;
  uint8_t second = first + 1;
  uint8_t count = tbd_ui_mode.window_count();

  draw_overlay_header();
  render_window(8, first, sub_page == first, 28);
  if (second < count) {
    render_window(36, second, sub_page == second, 28);
  } else {
    oled_display.fillRect(0, 36, 128, 28, BLACK);
  }

  uint8_t box_y = (sub_page == first) ? 8 : 36;
  oled_display.drawRect(0, box_y, 128, 28, WHITE);

  if (first > 0) {
    oled_display.fillTriangle(6, 32, 6, 38, 2, 35, WHITE);
  }
  if (second + 1 < count) {
    oled_display.fillTriangle(121, 32, 121, 38, 125, 35, WHITE);
  }
}

#endif // PLATFORM_TBD
