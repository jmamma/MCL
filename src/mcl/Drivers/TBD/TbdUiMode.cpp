#include "TbdUiMode.h"

#ifdef PLATFORM_TBD

#include "DeviceManager.h"
#include "GridPages.h"
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
#include "TBDTrack.h"
#include "TbdP4Command.h"
#include "TbdP4Realtime.h"
#include "GUI_hardware.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

TbdUiMode tbd_ui_mode;
TbdParamStripPage tbd_param_strip_page;
TbdParamOverlayPage tbd_param_overlay_page;

namespace {

constexpr uint16_t kParamOverlayHoldMs = 500;
constexpr uint32_t kPresetCommandTimeoutMs = 30000;
constexpr size_t kPresetListJsonBytes = 4096;

char preset_list_json[kPresetListJsonBytes];

void claim_tbd_ui_trig_leds() {
  if (grid_page.bank_popup) {
    grid_page.close_bank_popup();
    return;
  }
  mcl_gui.set_trigleds_local(0, TRIGLED_EXCLUSIVE);
}

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

void log_arrow_press(uint8_t source, uint8_t before, uint8_t after,
                     uint8_t count, TbdP4SoundData *sound) {
#ifdef DEBUGMODE
  DEBUG_PRINT("tbd_ui_arrow src ");
  DEBUG_PRINT((unsigned)source);
  DEBUG_PRINT(" page ");
  DEBUG_PRINT((unsigned)before);
  DEBUG_PRINT("->");
  DEBUG_PRINT((unsigned)after);
  DEBUG_PRINT(" count ");
  DEBUG_PRINT((unsigned)count);
  if (sound == nullptr) {
    DEBUG_PRINTLN(" sound null");
    return;
  }
  DEBUG_PRINT(" audio ");
  DEBUG_PRINT((unsigned)sound->audio_params.num_pages);
  DEBUG_PRINT(" mixer ");
  DEBUG_PRINT((unsigned)sound->mixer_params.num_pages);
  DEBUG_PRINT(" p4 ");
  DEBUG_PRINTLN((unsigned)sound->p4_track_index);
#endif
}

void copy_fixed_text(char *dst, size_t dst_len, const char *src) {
  if (dst == nullptr || dst_len == 0) return;
  dst[0] = '\0';
  if (src == nullptr) return;
  strncpy(dst, src, dst_len);
  dst[dst_len - 1] = '\0';
}

const char *skip_json_ws(const char *p, const char *end) {
  while (p < end && isspace((unsigned char)*p)) {
    p++;
  }
  return p;
}

const char *find_json_value(const char *begin, const char *end,
                            const char *key) {
  const size_t key_len = strlen(key);
  const char *p = begin;

  while (p < end) {
    if (*p != '"') {
      p++;
      continue;
    }

    if ((size_t)(end - p) < key_len + 3) {
      return nullptr;
    }

    if (strncmp(p + 1, key, key_len) == 0 && p[1 + key_len] == '"') {
      const char *v = skip_json_ws(p + key_len + 2, end);
      if (v < end && *v == ':') {
        return skip_json_ws(v + 1, end);
      }
    }
    p++;
  }

  return nullptr;
}

bool read_json_string(const char *begin, const char *end, const char *key,
                      char *dst, size_t dst_len) {
  if (dst == nullptr || dst_len == 0) {
    return false;
  }
  dst[0] = '\0';

  const char *p = find_json_value(begin, end, key);
  if (p == nullptr || p >= end || *p != '"') {
    return false;
  }
  p++;

  size_t copied = 0;
  bool escaped = false;
  while (p < end) {
    const char c = *p++;
    if (escaped) {
      if (copied + 1 < dst_len) {
        dst[copied++] = c;
      }
      escaped = false;
      continue;
    }
    if (c == '\\') {
      escaped = true;
      continue;
    }
    if (c == '"') {
      dst[copied] = '\0';
      return true;
    }
    if (copied + 1 < dst_len) {
      dst[copied++] = c;
    }
  }

  dst[copied] = '\0';
  return false;
}

const char *find_json_object_end(const char *begin, const char *end) {
  bool in_string = false;
  bool escaped = false;
  int depth = 0;

  for (const char *p = begin; p < end; p++) {
    const char c = *p;
    if (in_string) {
      if (escaped) {
        escaped = false;
      } else if (c == '\\') {
        escaped = true;
      } else if (c == '"') {
        in_string = false;
      }
      continue;
    }

    if (c == '"') {
      in_string = true;
    } else if (c == '{') {
      depth++;
    } else if (c == '}') {
      depth--;
      if (depth == 0) {
        return p + 1;
      }
    }
  }

  return nullptr;
}

const char *find_json_array_end(const char *begin, const char *end) {
  bool in_string = false;
  bool escaped = false;
  int depth = 0;

  for (const char *p = begin; p < end; p++) {
    const char c = *p;
    if (in_string) {
      if (escaped) {
        escaped = false;
      } else if (c == '\\') {
        escaped = true;
      } else if (c == '"') {
        in_string = false;
      }
      continue;
    }

    if (c == '"') {
      in_string = true;
    } else if (c == '[') {
      depth++;
    } else if (c == ']') {
      depth--;
      if (depth == 0) {
        return p + 1;
      }
    }
  }

  return nullptr;
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

void format_param_value(const TbdP4ParamDescriptor &param, uint8_t value,
                        char *dst, size_t dst_len) {
  put_int16_cell(tbd_p4_scale_lock_value(param, value14_from_normalized(value)),
                 dst, dst_len);
}

void render_window(uint8_t y_top, uint8_t window, bool active,
                   uint8_t row_height) {
  if (tbd_ui_mode.is_preset_page(window)) {
    tbd_ui_mode.render_preset_window(y_top, active, row_height);
    return;
  }

  Encoder proxies[4];
  TbdUiMode::ParamSlot slots[4];
  bool has_slot[4] = {};
  bool locked[4] = {};
  uint8_t display_value[4] = {};

  oled_display.fillRect(0, y_top, 128, row_height, BLACK);
  auto oldfont = oled_display.getFont();
  oled_display.setFont();
  oled_display.setTextColor(WHITE, BLACK);

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
  const uint8_t dial_y = y_top + 4;
  const uint8_t text_y = y_top + 18;
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

bool TbdUiMode::select_track(uint8_t track_idx) {
  if (!latched_) return false;

  MidiDevice *device = nullptr;
  uint8_t seq_slot = 1;

  if (device_idx_ == SLOT_PRIMARY) {
    if (mcl_cfg.grid_x_device != GRID_X_DEVICE_TBD ||
        track_idx >= mcl_seq.num_tbd_tracks) {
      return false;
    }
    last_md_track = track_idx;
    device = &TBD;
    seq_slot = 1;
  } else if (device_idx_ == SLOT_SECONDARY) {
#ifdef EXT_TRACKS
    if (mcl_cfg.grid_y_device != GRID_Y_DEVICE_TBD ||
        track_idx >= mcl_seq.num_midi_tracks ||
        track_idx >= NUM_EXT_TRACKS) {
      return false;
    }
    last_ext_track = track_idx;
    device = device_manager.secondary_device();
    seq_slot = 2;
#else
    return false;
#endif
  } else {
    return false;
  }

  const PageIndex pg = mcl.currentPage();
  if (pg == SEQ_STEP_PAGE || pg == SEQ_PTC_PAGE ||
      pg == SEQ_EXTSTEP_PAGE) {
    SeqPage::select_device_slot(seq_slot);
    if (device != nullptr) {
      seq_step_page.select_track(device, track_idx, false);
    }
  }

  TbdP4SoundData *sound = active_sound();
  if (sound != nullptr) {
    tbd_p4_realtime.set_active_track(sound->p4_track_index);
  }
  resync_from_sound();
  return true;
}

uint8_t TbdUiMode::window_count() const {
  const uint8_t driver_pages = tbd_p4_driver_param_page_count();
  TbdP4SoundData *sound = active_sound();
  if (sound == nullptr) return driver_pages == 0 ? 1 : driver_pages;

  uint8_t count = 1 + clamped_audio_pages(*sound) +
                  clamped_mixer_pages(*sound) + driver_pages;
  return count == 0 ? 1 : count;
}

bool TbdUiMode::is_preset_page(uint8_t window) const {
  return active_sound() != nullptr && window == 0;
}

void TbdUiMode::clear_preset_cache() {
  preset_group_count_ = 0;
  preset_count_ = 0;
  preset_cache_p4_track_ = 255;
  preset_cache_valid_ = false;
  preset_cache_failed_ = false;
  selected_group_ = 0;
  selected_preset_ = 0;
}

bool TbdUiMode::parse_preset_list_json(const char *json) {
  preset_group_count_ = 0;
  preset_count_ = 0;
  selected_group_ = 0;
  selected_preset_ = 0;

  if (json == nullptr) return false;
  const char *end = json + strlen(json);
  const char *groups = find_json_value(json, end, "presetgroups");
  if (groups == nullptr || groups >= end || *groups != '[') {
    return false;
  }

  const char *groups_end = find_json_array_end(groups, end);
  if (groups_end == nullptr) {
    return false;
  }

  const char *g = groups + 1;
  while (g < groups_end && preset_group_count_ < MAX_PRESET_GROUPS &&
         preset_count_ < MAX_PRESETS) {
    g = skip_json_ws(g, groups_end);
    if (g >= groups_end || *g == ']') break;
    if (*g != '{') {
      g++;
      continue;
    }

    const char *group_end = find_json_object_end(g, groups_end);
    if (group_end == nullptr) break;

    PresetGroup &group = preset_groups_[preset_group_count_];
    memset(&group, 0, sizeof(group));
    group.first_preset = preset_count_;
    read_json_string(g, group_end, "id", group.id, sizeof(group.id));
    read_json_string(g, group_end, "name", group.name, sizeof(group.name));
    if (group.name[0] == '\0') {
      copy_fixed_text(group.name, sizeof(group.name), group.id);
    }

    const char *presets = find_json_value(g, group_end, "presets");
    if (presets != nullptr && presets < group_end && *presets == '[') {
      const char *presets_end = find_json_array_end(presets, group_end);
      const char *p = presets + 1;
      while (presets_end != nullptr && p < presets_end &&
             preset_count_ < MAX_PRESETS) {
        p = skip_json_ws(p, presets_end);
        if (p >= presets_end || *p == ']') break;
        if (*p != '{') {
          p++;
          continue;
        }

        const char *preset_end = find_json_object_end(p, presets_end);
        if (preset_end == nullptr) break;

        PresetEntry &entry = presets_[preset_count_];
        memset(&entry, 0, sizeof(entry));
        entry.group = preset_group_count_;
        read_json_string(p, preset_end, "id", entry.id, sizeof(entry.id));
        read_json_string(p, preset_end, "name", entry.name,
                         sizeof(entry.name));
        if (entry.name[0] == '\0') {
          copy_fixed_text(entry.name, sizeof(entry.name), entry.id);
        }
        if (entry.id[0] != '\0') {
          preset_count_++;
          group.preset_count++;
        }
        p = preset_end;
      }
    }

    if (group.preset_count > 0) {
      preset_group_count_++;
    } else {
      preset_count_ = group.first_preset;
    }
    g = group_end;
  }

  return preset_group_count_ > 0 && preset_count_ > 0;
}

uint8_t TbdUiMode::selected_group_preset_count() const {
  if (selected_group_ >= preset_group_count_) return 0;
  return preset_groups_[selected_group_].preset_count;
}

uint8_t TbdUiMode::selected_global_preset_index() const {
  if (selected_group_ >= preset_group_count_) return 0;
  const PresetGroup &group = preset_groups_[selected_group_];
  if (selected_preset_ >= group.preset_count) return group.first_preset;
  return group.first_preset + selected_preset_;
}

void TbdUiMode::select_global_preset(uint8_t preset_index) {
  if (preset_count_ == 0) {
    selected_group_ = 0;
    selected_preset_ = 0;
    return;
  }
  if (preset_index >= preset_count_) {
    preset_index = preset_count_ - 1;
  }

  selected_group_ = presets_[preset_index].group;
  if (selected_group_ >= preset_group_count_) {
    selected_group_ = 0;
    selected_preset_ = 0;
    return;
  }

  const PresetGroup &group = preset_groups_[selected_group_];
  selected_preset_ = preset_index - group.first_preset;
  if (selected_preset_ >= group.preset_count) {
    selected_preset_ = 0;
  }
}

TbdUiMode::PresetEntry *TbdUiMode::selected_preset_entry() {
  if (selected_group_ >= preset_group_count_) return nullptr;
  const PresetGroup &group = preset_groups_[selected_group_];
  if (selected_preset_ >= group.preset_count) return nullptr;
  return &presets_[group.first_preset + selected_preset_];
}

const TbdUiMode::PresetEntry *TbdUiMode::selected_preset_entry() const {
  if (selected_group_ >= preset_group_count_) return nullptr;
  const PresetGroup &group = preset_groups_[selected_group_];
  if (selected_preset_ >= group.preset_count) return nullptr;
  return &presets_[group.first_preset + selected_preset_];
}

void TbdUiMode::sync_preset_selection_to_sound() {
  TbdP4SoundData *sound = active_sound();
  selected_group_ = 0;
  selected_preset_ = 0;
  if (sound != nullptr && sound->preset_id[0] != '\0') {
    for (uint8_t i = 0; i < preset_count_; i++) {
      if (strncmp(presets_[i].id, sound->preset_id,
                  sizeof(presets_[i].id)) == 0) {
        selected_group_ = presets_[i].group;
        selected_preset_ = i - preset_groups_[selected_group_].first_preset;
        break;
      }
    }
  }
  enc[0].setValue(selected_group_);
  enc[1].setValue(selected_global_preset_index());
  enc[2].setValue(0);
  enc[3].setValue(0);
}

bool TbdUiMode::ensure_preset_cache() {
  TbdP4SoundData *sound = active_sound();
  if (sound == nullptr) return false;
  if (preset_cache_p4_track_ == sound->p4_track_index) {
    if (preset_cache_valid_) return true;
    if (preset_cache_failed_) return false;
  }

  clear_preset_cache();
  preset_cache_p4_track_ = sound->p4_track_index;
  preset_list_json[0] = '\0';

  tbd_p4_command.init();
  const bool got_list = tbd_p4_command.get_macro_sound_preset_list(
      sound->p4_track_index, preset_list_json, sizeof(preset_list_json),
      kPresetCommandTimeoutMs);
  if (!got_list || !parse_preset_list_json(preset_list_json)) {
#ifdef DEBUGMODE
    DEBUG_PRINT("tbd_ui preset list failed p4=");
    DEBUG_PRINTLN((unsigned)sound->p4_track_index);
#endif
    preset_cache_failed_ = true;
    return false;
  }

  preset_cache_valid_ = true;
  preset_cache_failed_ = false;
  sync_preset_selection_to_sound();
  return true;
}

void TbdUiMode::render_preset_window(uint8_t y_top, bool active,
                                     uint8_t row_height) {
  oled_display.fillRect(0, y_top, 128, row_height, BLACK);
  auto oldfont = oled_display.getFont();
  oled_display.setFont();
  oled_display.setTextColor(WHITE, BLACK);

  TbdP4SoundData *sound = active_sound();
  char line[24];
  oled_display.setCursor(2, y_top + 2);
  if (sound == nullptr) {
    oled_display.print("PRESET");
    oled_display.setCursor(2, y_top + 16);
    oled_display.print("NO TRACK");
    oled_display.setFont(oldfont);
    return;
  }

  bool ready = ensure_preset_cache();
  snprintf(line, sizeof(line), "PRESET T%02u", (unsigned)sound->p4_track_index);
  oled_display.print(line);

  const PresetEntry *entry = selected_preset_entry();
  const bool current =
      entry != nullptr &&
      strncmp(entry->id, sound->preset_id, sizeof(entry->id)) == 0;
  oled_display.setCursor(98, y_top + 2);
  oled_display.print(preset_apply_in_progress_ ? "..." :
                     (preset_apply_failed_ ? "ERR" :
                      (current ? "CUR" : "NEW")));

  if (!ready || entry == nullptr) {
    oled_display.setCursor(2, y_top + 16);
    oled_display.print("NO PRESETS");
    oled_display.setFont(oldfont);
    return;
  }

  char group_text[16];
  char preset_text[16];
  copy_text(preset_groups_[selected_group_].name, group_text,
            sizeof(group_text), 13);
  copy_text(entry->name, preset_text, sizeof(preset_text), 13);
  oled_display.setCursor(2, y_top + 12);
  oled_display.print(group_text);
  oled_display.setCursor(2, y_top + 22);
  oled_display.print(preset_text);

  if (active) {
    oled_display.drawFastVLine(123, y_top + 8, row_height - 10, WHITE);
  }
  oled_display.setFont(oldfont);
}

bool TbdUiMode::param_slot(uint8_t window, uint8_t encoder_idx,
                           ParamSlot &slot) const {
  slot = ParamSlot();
  if (encoder_idx >= 4) return false;

  TbdP4SoundData *sound = active_sound();
  if (sound != nullptr) {
    if (window == 0) return false;
    window--;
  }

  const uint8_t audio_pages = sound == nullptr ? 0 : clamped_audio_pages(*sound);
  if (window < audio_pages) {
    uint8_t param_idx = window * 4 + encoder_idx;
    if (param_idx >= TBD_P4_AUDIO_PARAM_COUNT) return false;
    slot.sound = sound;
    slot.lock_param = param_idx;
    slot.param = mutable_param_for_lock(*sound, slot.lock_param);
    return is_tbd_param_visible(slot.param);
  }

  window -= audio_pages;
  const uint8_t mixer_pages = sound == nullptr ? 0 : clamped_mixer_pages(*sound);
  if (window < mixer_pages) {
    uint8_t mixer_idx = window * 4 + encoder_idx;
    if (mixer_idx >= TBD_P4_MIXER_PARAM_COUNT) return false;
    slot.sound = sound;
    slot.lock_param = TBD_P4_LOCK_MIXER_PARAM_BASE + mixer_idx;
    slot.param = mutable_param_for_lock(*sound, slot.lock_param);
    return is_tbd_param_visible(slot.param);
  }

  window -= mixer_pages;
  if (window < tbd_p4_driver_param_page_count()) {
    uint8_t driver_idx = window * 4 + encoder_idx;
    slot.driver_param = driver_idx;
    slot.param = tbd_p4_driver_param(driver_idx);
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
  if (slot.driver_param != 255) {
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
    claim_tbd_ui_trig_leds();
    show_fullscreen();
    return true;
  }

  latched_ = true;
  device_idx_ = device_idx;
  claim_tbd_ui_trig_leds();
  GUI_hardware.led.set_tbd_driver_leds(device_idx_ == SLOT_PRIMARY,
                                       device_idx_ == SLOT_SECONDARY);
  sub_page_ = min(sub_page_, (uint8_t)(window_count() - 1));
  resync_from_sound();
  show_fullscreen();
  return true;
}

void TbdUiMode::disable() {
  const bool was_latched = latched_;
  latched_ = false;
  device_idx_ = SLOT_NONE;
  bound_device_idx_ = SLOT_NONE;
  bound_track_ = 255;
  bound_sub_page_ = 255;
  ui_button_pressed_ = false;
  ui_button_hold_handled_ = false;
  GUI_hardware.led.set_tbd_driver_leds(false, false);
  if (was_latched) {
    mcl_gui.reset_trigleds();
  }
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

bool TbdUiMode::is_collapsed() const {
  return latched_ && GUI.overlay == &tbd_param_strip_page;
}

void TbdUiMode::handle_ui_slot_button(bool pressed) {
  if (pressed) {
    ui_button_press_ms_ = read_clock_ms();
    ui_button_pressed_ = true;
    ui_button_hold_handled_ = false;
  } else {
    const bool short_press =
        ui_button_pressed_ && !ui_button_hold_handled_ &&
        clock_diff(ui_button_press_ms_, read_clock_ms()) <=
            kParamOverlayHoldMs;
    ui_button_pressed_ = false;
    if (short_press && GUI.overlay == &tbd_param_overlay_page &&
        is_preset_page(sub_page_)) {
      apply_selected_preset();
    }
    ui_button_hold_handled_ = false;
  }
}

void TbdUiMode::poll_ui_button_hold() {
  if (!latched_ || !ui_button_pressed_ || ui_button_hold_handled_) return;
  if (clock_diff(ui_button_press_ms_, read_clock_ms()) <=
      kParamOverlayHoldMs) {
    return;
  }
  if (GUI.overlay == &tbd_param_overlay_page) {
    show_strip();
  } else {
    show_fullscreen();
  }
  ui_button_hold_handled_ = true;
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

void TbdUiMode::select_sub_page_half(bool lower_half) {
  uint8_t count = window_count();
  if (count <= 1) return;
  uint8_t next = (uint8_t)((sub_page_ & 0xFE) | (lower_half ? 1 : 0));
  if (next >= count) return;
  if (next == sub_page_) return;
  sub_page_ = next;
  resync_from_sound();
}

bool TbdUiMode::handle_event(gui_event_t *event) {
  const bool entry_arrow_trace =
      EVENT_BUTTON(event) &&
      event->source >= ButtonsClass::FUNC_BUTTON6 &&
      event->source <= ButtonsClass::FUNC_BUTTON9;
  if (entry_arrow_trace) {
    DEBUG_PRINT("    TbdUiMode::handle_event latched=");
    DEBUG_PRINT((unsigned)latched_);
    DEBUG_PRINT(" mask=");
    DEBUG_PRINTLN((unsigned)event->mask);
  }

  if (!latched_ || !EVENT_BUTTON(event)) {
    if (entry_arrow_trace) DEBUG_PRINTLN("    -> reject (!latched/!EVENT_BUTTON)");
    return false;
  }
  if (event->mask != EVENT_BUTTON_PRESSED &&
      event->mask != EVENT_BUTTON_RELEASED) {
    return false;
  }
  if (is_collapsed()) {
    return false;
  }

  const bool is_press = event->mask == EVENT_BUTTON_PRESSED;
  const bool is_param_arrow =
      event->source == ButtonsClass::FUNC_BUTTON6 ||
      event->source == ButtonsClass::FUNC_BUTTON7 ||
      event->source == ButtonsClass::FUNC_BUTTON8 ||
      event->source == ButtonsClass::FUNC_BUTTON9;

  if (!is_param_arrow) {
    return false;
  }

  if (encoder_passthrough_page()) {
    if (entry_arrow_trace) DEBUG_PRINTLN("    -> reject (encoder_passthrough_page)");
    return false;
  }
  if (entry_arrow_trace) {
    DEBUG_PRINT("    -> handling, count=");
    DEBUG_PRINT((unsigned)window_count());
    DEBUG_PRINT(" sub_page=");
    DEBUG_PRINTLN((unsigned)sub_page_);
  }

  const uint8_t before_sub_page = sub_page_;
  const uint8_t count = window_count();
  TbdP4SoundData *sound = active_sound();

  if (event->source == ButtonsClass::FUNC_BUTTON6 ||
      event->source == ButtonsClass::FUNC_BUTTON8) {
    if (is_press) {
      if (GUI.overlay != &tbd_param_overlay_page) {
        show_fullscreen();
      }
      select_sub_page_half(event->source == ButtonsClass::FUNC_BUTTON8);
      log_arrow_press(event->source, before_sub_page, sub_page_, count, sound);
    }
    return true;
  }

  if (event->source == ButtonsClass::FUNC_BUTTON7 ||
      event->source == ButtonsClass::FUNC_BUTTON9) {
    if (is_press) {
      if (GUI.overlay != &tbd_param_overlay_page) {
        show_fullscreen();
      }
      int8_t delta = event->source == ButtonsClass::FUNC_BUTTON7 ? -2 : 2;
      if (BUTTON_DOWN(ButtonsClass::FUNC_BUTTON5)) {
        delta = event->source == ButtonsClass::FUNC_BUTTON7 ? -1 : 1;
      }
      move_sub_page(delta);
      log_arrow_press(event->source, before_sub_page, sub_page_, count, sound);
    }
    return true;
  }

  return false;
}

bool TbdUiMode::encoder_passthrough_page() const {
  PageIndex pg = mcl.currentPage();
  return pg == PAGE_SELECT_PAGE || pg == BANK_POPUP_PAGE;
}

void TbdUiMode::resync_from_sound() {
  if (is_preset_page(sub_page_)) {
    ensure_preset_cache();
    sync_preset_selection_to_sound();
  } else {
    for (uint8_t i = 0; i < 4; i++) {
      ParamSlot slot;
      if (param_slot(sub_page_, i, slot) &&
          is_tbd_param_visible(slot.param)) {
        enc[i].setValue(normalized_value(*slot.param));
      } else {
        enc[i].setValue(0);
      }
    }
  }
  bound_device_idx_ = device_idx_;
  bound_track_ = active_track_index();
  bound_sub_page_ = sub_page_;
}

void TbdUiMode::poll_preset_encoders(encoder_t *snapshot,
                                     uint16_t now) {
  if (!ensure_preset_cache() || preset_group_count_ == 0) {
    return;
  }

  enc[0].update(&snapshot[0]);
  if (enc[0].cur >= preset_group_count_) {
    enc[0].setValue(preset_group_count_ - 1);
  }
  uint8_t next_group = (uint8_t)enc[0].cur;
  if (next_group != selected_group_) {
    selected_group_ = next_group;
    selected_preset_ = 0;
    enc[1].setValue(preset_groups_[selected_group_].first_preset);
    enc_used_clock_[0] = now ? now : 1;
    preset_apply_failed_ = false;
  }
  enc[0].old = enc[0].cur;

  if (preset_count_ == 0) {
    enc[1].setValue(0);
    return;
  }

  enc[1].update(&snapshot[1]);
  if (enc[1].cur >= preset_count_) {
    enc[1].setValue(preset_count_ - 1);
  }
  uint8_t next_preset = (uint8_t)enc[1].cur;
  if (next_preset != selected_global_preset_index()) {
    select_global_preset(next_preset);
    enc[0].setValue(selected_group_);
    enc_used_clock_[1] = now ? now : 1;
    preset_apply_failed_ = false;
  }
  enc[1].old = enc[1].cur;
}

bool TbdUiMode::apply_selected_preset() {
  if (preset_apply_in_progress_) {
    return false;
  }
  TbdP4SoundData *sound = active_sound();
  if (sound == nullptr || !ensure_preset_cache()) {
    return false;
  }
  const PresetEntry *entry = selected_preset_entry();
  if (entry == nullptr || entry->id[0] == '\0') {
    return false;
  }

  preset_apply_in_progress_ = true;
  preset_apply_failed_ = false;

  TbdP4SoundData next = *sound;
  next.audio_params.clear();
  next.preset_name[0] = '\0';
  next.macro_id[0] = '\0';
  next.machine_id[0] = '\0';
  next.rom_bank = 0xFF;
  next.sample_slice = -1;
  copy_fixed_text(next.preset_id, sizeof(next.preset_id), entry->id);

#ifdef DEBUGMODE
  DEBUG_PRINT("tbd_ui apply preset p4=");
  DEBUG_PRINT((unsigned)next.p4_track_index);
  DEBUG_PRINT(" id=");
  DEBUG_PRINTLN(next.preset_id);
#endif

  const bool hydrated = TBD.hydrate_p4_sound(next);
  const bool loaded =
      hydrated &&
      tbd_p4_command.load_track_sound_preset(next.p4_track_index,
                                             next.preset_id,
                                             next.rom_bank,
                                             next.sample_slice,
                                             kPresetCommandTimeoutMs);
  if (loaded) {
    *sound = next;
    tbd_p4_realtime.set_active_track(sound->p4_track_index);
    tbd_p4_send_sound_state(*sound);
    tbd_mark_p4_sound_applied(*sound);
    resync_from_sound();
  } else {
    preset_apply_failed_ = true;
  }

  preset_apply_in_progress_ = false;
#ifdef DEBUGMODE
  DEBUG_PRINT("tbd_ui apply preset ok=");
  DEBUG_PRINTLN(loaded ? 1 : 0);
#endif
  return loaded;
}

void TbdUiMode::send_param(uint8_t encoder_idx) {
  ParamSlot slot;
  if (!param_slot(sub_page_, encoder_idx, slot) ||
      !slot.param || !slot.param->is_sendable()) {
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

  if (slot.driver_param != 255) {
    tbd_p4_send_driver_param(slot.driver_param);
    return;
  }

  if (!slot.sound) return;

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
  if (slot.driver_param != 255) {
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
  if (!latched_) return;
  poll_ui_button_hold();
  if (encoder_passthrough_page()) return;

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
  if (is_preset_page(sub_page_)) {
    poll_preset_encoders(snapshot, now);
    return;
  }

  for (uint8_t i = 0; i < 4; i++) {
    ParamSlot slot;
    if (!param_slot(sub_page_, i, slot)) continue;

    enc[i].update(&snapshot[i]);
    if (enc[i].hasChanged()) {
      send_param(i);
      enc[i].old = enc[i].cur;
      enc_used_clock_[i] = now ? now : 1;
    }
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

  render_window(0, first, sub_page == first, 32);
  if (second < count) {
    render_window(32, second, sub_page == second, 32);
  } else {
    oled_display.fillRect(0, 32, 128, 32, BLACK);
  }

  uint8_t box_y = (sub_page == first) ? 0 : 32;
  oled_display.drawRect(0, box_y, 128, 32, WHITE);

  if (first > 0) {
    oled_display.fillTriangle(6, 28, 6, 34, 2, 31, WHITE);
  }
  if (second + 1 < count) {
    oled_display.fillTriangle(121, 28, 121, 34, 125, 31, WHITE);
  }
}

#endif // PLATFORM_TBD
