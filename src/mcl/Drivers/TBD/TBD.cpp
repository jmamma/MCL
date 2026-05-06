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
#include <stdio.h>

namespace {

struct P4BootPreset {
  uint8_t track_index;
  const char *preset_id;
};

constexpr uint32_t kP4PresetRetryMs = 2000;
constexpr uint32_t kP4PresetReadyProbeMs = 100;
constexpr uint32_t kP4PresetCommandTimeoutMs = 3000;

const P4BootPreset kP4BootPresets[] = {
    {0, "db-all-def"},    {1, "fmb-all-def"},
    {2, "ds-all-def"},    {3, "hh1-all-def"},
    {4, "rs-all-def"},    {5, "cl-all-def"},
    {6, "ro-all-def"},    {7, "ro-all-def"},
    {8, "td3-all-def"},   {9, "td3-all-def"},
    {10, "mo-all-def"},   {11, "wtosc-all-def"},
    {12, "ro-all-def"},   {13, "ro-all-def"},
    {14, "pp-all-def"},   {15, "inp-all-def"},
};

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
  uint8_t grid_idx = 1;
  GridDeviceTrack gdt;

  for (uint8_t i = 0; i < NUM_EXT_TRACKS; i++) {
    const auto &def = tbd_track_default_for_slot(i);
    mcl_seq.ext_tracks[i].channel = def.midi_channel;
    gdt.init(TBD_TRACK_TYPE, GROUP_DEV, device_idx, &(mcl_seq.ext_tracks[i]));
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

  tbd_p4_command.announce_app("MCL", 0, kP4PresetCommandTimeoutMs);

  for (const auto &preset : kP4BootPresets) {
    if (!tbd_p4_command.load_track_sound_preset(
            preset.track_index, preset.preset_id, 0xFF, -1,
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
