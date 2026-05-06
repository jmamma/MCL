#include "TBD.h"

#ifdef PLATFORM_TBD

#include "MCL.h"
#include "MCLGUI.h"
#include "MCLSeq.h"
#include "MidiDeviceGrid.h"
#include "MidiSetup.h"
#include "TbdP4Realtime.h"
#include <stdio.h>

namespace {

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
    snprintf(line, sizeof(line), "P4 A%u R%u D%u E%lu",
             stats.p4_alive ? 1 : 0,
             stats.p4_ready_pin ? 1 : 0,
             stats.dma_ready ? 1 : 0,
             (unsigned long)(stats.error_count % 10000));
    oled_display.println(line);

    snprintf(line, sizeof(line), "FR %lu/%lu SQ %u",
             (unsigned long)(stats.tx_frames % 10000),
             (unsigned long)(stats.rx_frames % 10000),
             stats.last_response_sequence);
    oled_display.println(line);

    snprintf(line, sizeof(line), "BY %lu/%lu",
             (unsigned long)(stats.tx_midi_bytes % 100000),
             (unsigned long)(stats.rx_midi_bytes % 100000));
    oled_display.println(line);

    snprintf(line, sizeof(line), "DR %lu/%lu TO %lu",
             (unsigned long)(stats.dropped_tx_bytes % 10000),
             (unsigned long)(stats.dropped_rx_bytes % 10000),
             (unsigned long)(stats.dma_timeout_count % 10000));
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
}

void TbdDevice::init_grid_devices(uint8_t device_idx) {
  uint8_t grid_idx = 1;
  GridDeviceTrack gdt;

  for (uint8_t i = 0; i < NUM_EXT_TRACKS; i++) {
    gdt.init(EXT_TRACK_TYPE, GROUP_DEV, device_idx, &(mcl_seq.ext_tracks[i]));
    add_track_to_grid(grid_idx, i, &gdt);
  }
}

void TbdDevice::note_on(uint8_t note) {
  if (port != UARTP4_PORT || uart == nullptr) return;
  note_off();
  active_note_ = note;
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
