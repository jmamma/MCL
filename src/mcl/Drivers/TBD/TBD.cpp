#include "TBD.h"

#ifdef PLATFORM_TBD

#include "MCL.h"
#include "MCLGUI.h"
#include "MCLSeq.h"
#include "MidiDeviceGrid.h"
#include "MidiSetup.h"
#include "TBDTrack.h"
#include "TbdP4Command.h"
#include "TbdP4Realtime.h"
#include <Arduino.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {

struct P4BootPreset {
  uint8_t track_index;
  char preset_id[TBD_PRESET_ID_LEN];
  uint8_t rom_bank;
  int32_t sample_slice;
};

struct P4BootPresetFallback {
  uint8_t track_index;
  const char *preset_id;
};

constexpr uint32_t kP4PresetRetryMs = 2000;
constexpr uint32_t kP4PresetReadyProbeMs = 100;
constexpr uint32_t kP4PresetCommandTimeoutMs = 3000;
constexpr size_t kP4SoundTrackCount = 16;
constexpr size_t kP4TrackDefaultsJsonBytes = 4096;
constexpr uint8_t kP4DefaultRomBank = 0xFF;
constexpr int32_t kP4DefaultSampleSlice = -1;

const P4BootPresetFallback kP4BootPresetFallbacks[] = {
    {0, "db-all-def"},    {1, "fmb-all-def"},
    {2, "ds-all-def"},    {3, "hh1-all-def"},
    {4, "rs-all-def"},    {5, "cl-all-def"},
    {6, "ro-all-def"},    {7, "ro-all-def"},
    {8, "td3-all-def"},   {9, "td3-all-def"},
    {10, "mo-all-def"},   {11, "wtosc-all-def"},
    {12, "ro-all-def"},   {13, "ro-all-def"},
    {14, "pp-all-def"},   {15, "inp-all-def"},
};

P4BootPreset p4_boot_presets[kP4SoundTrackCount];
char p4_track_defaults_json[kP4TrackDefaultsJsonBytes];

void copy_preset_id(char *dst, const char *src) {
  if (src == nullptr) {
    dst[0] = '\0';
    return;
  }
  strncpy(dst, src, TBD_PRESET_ID_LEN);
  dst[TBD_PRESET_ID_LEN - 1] = '\0';
}

void reset_p4_boot_presets_to_fallback() {
  for (auto &preset : p4_boot_presets) {
    preset.track_index = 0;
    preset.preset_id[0] = '\0';
    preset.rom_bank = kP4DefaultRomBank;
    preset.sample_slice = kP4DefaultSampleSlice;
  }

  for (const auto &fallback : kP4BootPresetFallbacks) {
    if (fallback.track_index >= kP4SoundTrackCount) {
      continue;
    }
    auto &preset = p4_boot_presets[fallback.track_index];
    preset.track_index = fallback.track_index;
    preset.rom_bank = kP4DefaultRomBank;
    preset.sample_slice = kP4DefaultSampleSlice;
    copy_preset_id(preset.preset_id, fallback.preset_id);
  }
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

bool read_json_int(const char *begin, const char *end, const char *key,
                   long *value) {
  const char *p = find_json_value(begin, end, key);
  if (p == nullptr || p >= end) {
    return false;
  }

  char *parse_end = nullptr;
  const long parsed = strtol(p, &parse_end, 10);
  if (parse_end == p || parse_end > end) {
    return false;
  }

  *value = parsed;
  return true;
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

bool parse_p4_track_defaults(const char *json, P4BootPreset *presets,
                             size_t preset_count, char *kit_id,
                             size_t kit_id_len) {
  if (json == nullptr || presets == nullptr || preset_count == 0) {
    return false;
  }

  const char *end = json + strlen(json);
  if (kit_id != nullptr && kit_id_len > 0) {
    read_json_string(json, end, "kit", kit_id, kit_id_len);
  }

  const char *tracks = find_json_value(json, end, "tracks");
  if (tracks == nullptr || tracks >= end || *tracks != '[') {
    return false;
  }

  bool parsed_any = false;
  const char *p = tracks + 1;
  while (p < end) {
    p = skip_json_ws(p, end);
    if (p >= end || *p == ']') {
      break;
    }
    if (*p != '{') {
      p++;
      continue;
    }

    const char *obj_end = find_json_object_end(p, end);
    if (obj_end == nullptr) {
      break;
    }

    long track_index = -1;
    char preset_id[TBD_PRESET_ID_LEN];
    if (read_json_int(p, obj_end, "index", &track_index) &&
        read_json_string(p, obj_end, "preset", preset_id,
                         sizeof(preset_id)) &&
        track_index >= 0 && (size_t)track_index < preset_count &&
        preset_id[0] != '\0') {
      auto &preset = presets[track_index];
      preset.track_index = (uint8_t)track_index;
      copy_preset_id(preset.preset_id, preset_id);

      long sample_bank = kP4DefaultRomBank;
      long sample_slice = kP4DefaultSampleSlice;
      if (read_json_int(p, obj_end, "sampleBank", &sample_bank) &&
          sample_bank >= 0 && sample_bank <= 0xFE) {
        preset.rom_bank = (uint8_t)sample_bank;
      }
      if (read_json_int(p, obj_end, "sampleSlice", &sample_slice)) {
        preset.sample_slice = (int32_t)sample_slice;
      }
      parsed_any = true;
    }

    p = obj_end;
  }

  return parsed_any;
}

uint8_t find_p4_kit_index_for_id(const char *kit_json, const char *kit_id) {
  if (kit_json == nullptr || kit_id == nullptr || kit_id[0] == '\0') {
    return 0;
  }

  const char *end = kit_json + strlen(kit_json);
  const char *kits = find_json_value(kit_json, end, "kits");
  if (kits == nullptr || kits >= end || *kits != '[') {
    return 0;
  }

  uint8_t index = 0;
  const char *p = kits + 1;
  while (p < end) {
    p = skip_json_ws(p, end);
    if (p >= end || *p == ']') {
      break;
    }
    if (*p != '{') {
      p++;
      continue;
    }

    const char *obj_end = find_json_object_end(p, end);
    if (obj_end == nullptr) {
      break;
    }

    char id[24];
    if (read_json_string(p, obj_end, "id", id, sizeof(id)) &&
        strcmp(id, kit_id) == 0) {
      return index;
    }
    if (index < 0xFF) {
      index++;
    }
    p = obj_end;
  }

  return 0;
}

class TbdP4DiagOverlay : public LightPage {
public:
  virtual void display() override {
    TbdP4RealtimeStats stats;
    tbd_p4_realtime.get_stats(stats);

    constexpr uint8_t y = 32;
    oled_display.fillRect(0, y, 128, 32, BLACK);
    oled_display.drawFastHLine(0, y, 128, WHITE);

    char line[22];
    oled_display.setCursor(0, y + 2);
    snprintf(line, sizeof(line), "P4 A%u S%u R%u E%lu",
             stats.p4_alive ? 1 : 0,
             stats.p4_sync_seen ? 1 : 0,
             stats.p4_ready_pin ? 1 : 0,
             (unsigned long)(stats.error_count % 10000));
    oled_display.println(line);

    snprintf(line, sizeof(line), "D%u WS%lu NR%lu F%lu",
             stats.dma_ready ? 1 : 0,
             (unsigned long)(stats.ws_sync_count % 1000),
             (unsigned long)(stats.p4_not_ready_count % 1000),
             (unsigned long)(stats.fingerprint_errors % 1000));
    oled_display.println(line);

    snprintf(line, sizeof(line), "L%lu C%lu SQ%lu MS%lu",
             (unsigned long)(stats.length_errors % 1000),
             (unsigned long)(stats.crc_errors % 1000),
             (unsigned long)(stats.sequence_errors % 1000),
             (unsigned long)(stats.missed_ws_sync_count % 1000));
    oled_display.println(line);

    snprintf(line, sizeof(line), "FR%lu/%lu TO%lu DU%lu",
             (unsigned long)(stats.tx_frames % 1000),
             (unsigned long)(stats.rx_frames % 1000),
             (unsigned long)(stats.dma_timeout_count % 1000),
             (unsigned long)(stats.dma_unavailable_count % 1000));
    oled_display.println(line);
  }
};

TbdP4DiagOverlay tbd_p4_diag_overlay;

} // namespace

TbdDevice::TbdDevice() : MidiDevice(&MidiP4, "TBD", DEVICE_MIDI, false) {
  port = UARTP4_PORT;
}

bool TbdDevice::probe() {
  connected = true;
  return true;
}

void TbdDevice::on_connection(uint8_t device_idx) {
  port = UARTP4_PORT;
  midi = &MidiP4;
  uart = MidiP4.uart;
  connected = true;
  init_grid_devices(device_idx);
  load_default_p4_presets();
}

void TbdDevice::init_grid_devices(uint8_t device_idx) {
  GridDeviceTrack gdt;

#if defined(PLATFORM_TBD)
  for (uint8_t i = 0; i < mcl_seq.num_tbd_tracks; i++) {
    gdt.init(TBD_TRACK_TYPE, GROUP_DEV, device_idx, &(mcl_seq.tbd_tracks[i]));
    add_track_to_grid(0, i, &gdt);
  }
#endif

  uint8_t grid_idx = 1;
  for (uint8_t i = 0; i < NUM_EXT_TRACKS; i++) {
    const auto &def = tbd_track_default_for_slot(i);
    mcl_seq.ext_tracks[i].channel = def.midi_channel;
    gdt.init(TBD_MIDI_TRACK_TYPE, GROUP_DEV, device_idx, &(mcl_seq.ext_tracks[i]));
    add_track_to_grid(grid_idx, i, &gdt);
  }
}

bool TbdDevice::load_default_p4_presets() {
  if (p4_defaults_loaded_) {
    return true;
  }

  const uint32_t now = millis();
  if (p4_defaults_last_attempt_ms_ != 0 &&
      now - p4_defaults_last_attempt_ms_ < kP4PresetRetryMs) {
    return false;
  }
  p4_defaults_last_attempt_ms_ = now;

  tbd_p4_command.init();
  if (!tbd_p4_command.wait_ready(kP4PresetReadyProbeMs)) {
    return false;
  }

  if (!tbd_p4_command.announce_app("MCL", 0, kP4PresetCommandTimeoutMs)) {
    return false;
  }

  reset_p4_boot_presets_to_fallback();

  char kit_id[32] = {0};
  if (tbd_p4_command.get_track_default_presets(
          p4_track_defaults_json, sizeof(p4_track_defaults_json), nullptr,
          kP4PresetCommandTimeoutMs)) {
    parse_p4_track_defaults(p4_track_defaults_json, p4_boot_presets,
                            kP4SoundTrackCount, kit_id, sizeof(kit_id));
  }

  if (kit_id[0] != '\0' &&
      tbd_p4_command.get_kit_index_json(
          p4_track_defaults_json, sizeof(p4_track_defaults_json),
          kP4PresetCommandTimeoutMs)) {
    const uint8_t kit_index =
        find_p4_kit_index_for_id(p4_track_defaults_json, kit_id);
    if (!tbd_p4_command.set_active_sample_kit(kit_index,
                                              kP4PresetCommandTimeoutMs)) {
      return false;
    }
  }

  for (const auto &preset : p4_boot_presets) {
    if (preset.preset_id[0] == '\0') {
      continue;
    }
    tbd_update_track_default_from_p4(preset.track_index, preset.preset_id,
                                     preset.rom_bank, preset.sample_slice);
    if (!tbd_p4_command.load_track_sound_preset(
            preset.track_index, preset.preset_id, preset.rom_bank,
            preset.sample_slice,
            kP4PresetCommandTimeoutMs)) {
      return false;
    }
  }

  p4_defaults_loaded_ = true;
  return true;
}

void TbdDevice::note_on(uint8_t note) {
  if (port != UARTP4_PORT || uart == nullptr) return;
  note_off();
  active_note_ = note;
  tbd_p4_realtime.set_active_track(tbd_track_default_for_slot(0).p4_track_index);
  uart->sendNoteOn(0, note, 100);
}

void TbdDevice::note_off() {
  if (active_note_ == 255 || uart == nullptr) return;
  uart->sendNoteOff(0, active_note_);
  active_note_ = 255;
}

bool TbdDevice::enter_ui(gui_event_t *event) {
  if (port != UARTP4_PORT) return false;
  if (!EVENT_BUTTON(event) ||
      event->source != ButtonsClass::TBD_BUTTON_TR ||
      event->mask != EVENT_BUTTON_PRESSED) {
    return false;
  }

  if (diag_active_) {
    exit_ui();
  } else {
    diag_active_ = true;
    GUI.setOverlay(&tbd_p4_diag_overlay);
  }
  return true;
}

bool TbdDevice::handle_ui_event(gui_event_t *event) {
  if (!diag_active_) return false;
  if (!EVENT_BUTTON(event)) return false;

  if (event->source == ButtonsClass::TBD_BUTTON_TR) {
    if (event->mask == EVENT_BUTTON_PRESSED) exit_ui();
    return true;
  }

  if (event->source >= ButtonsClass::TRIG_BUTTON1 &&
      event->source < ButtonsClass::TRIG_BUTTON1 + 16) {
    uint8_t note = (uint8_t)(36 + event->source - ButtonsClass::TRIG_BUTTON1);
    if (event->mask == EVENT_BUTTON_PRESSED) {
      note_on(note);
    } else if (event->mask == EVENT_BUTTON_RELEASED) {
      note_off();
    }
    return true;
  }

  return false;
}

void TbdDevice::ui_loop() {
  if (!p4_defaults_loaded_) {
    load_default_p4_presets();
  }

  if (diag_active_ && GUI.overlay == nullptr) {
    GUI.setOverlay(&tbd_p4_diag_overlay);
  }
}

bool TbdDevice::is_ui_active() {
  return diag_active_;
}

void TbdDevice::exit_ui() {
  note_off();
  diag_active_ = false;
  if (GUI.overlay == &tbd_p4_diag_overlay) {
    GUI.clearOverlay();
  }
}

#endif // PLATFORM_TBD
