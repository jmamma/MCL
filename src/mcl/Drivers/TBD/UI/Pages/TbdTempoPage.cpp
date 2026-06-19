#include "TBD/UI/Pages/TbdTempoPage.h"

#ifdef PLATFORM_TBD

#include "MCL.h"
#include "MCLGfx.h"
#include "MCLSysConfig.h"
#include "MidiClock.h"
#include "MidiSetup.h"
#include "helpers.h"
#include <stdio.h>
#include <string.h>

TbdTempoPage tbd_tempo_page;

TbdTempoPage::TbdTempoPage()
    : LightPage(&tempo_encoder_),
      tempo_encoder_(127, -127, 1) {}

void TbdTempoPage::begin(bool tap) {
  tap_on_open_ = tap;
  if (!active_) {
    GUI.setOverlay(this);
  }
  if (tap) {
    tap_mode_ = true;
    handle_tap();
  } else if (!tap_on_open_) {
    tap_mode_ = false;
  }
}

void TbdTempoPage::init() {
  active_ = true;
  sync_from_clock();
  tempo_encoder_.cur = 0;
  tempo_encoder_.old = 0;
  tap_mode_ = tap_on_open_;
  reset_taps();
  tap_on_open_ = false;
}

void TbdTempoPage::cleanup() {
  active_ = false;
  tap_mode_ = false;
}

void TbdTempoPage::close() {
  GUI.clearOverlay();
}

bool TbdTempoPage::tempo_edit_allowed() const {
  return mcl_cfg.clock_rec == MIDI_CLOCK_SOURCE_INTERNAL;
}

void TbdTempoPage::sync_from_clock() {
  float bpm = MidiClock.get_tempo();
  if (bpm < 1.0f) {
    bpm = mcl_cfg.tempo;
  }
  int16_t tenths = (int16_t)(bpm * 10.0f + 0.5f);
  if (tenths < (int16_t)kMinTempoTenths) tenths = kMinTempoTenths;
  if (tenths > (int16_t)kMaxTempoTenths) tenths = kMaxTempoTenths;
  tempo_tenths_ = (uint16_t)tenths;
}

void TbdTempoPage::set_tempo_tenths(int16_t tenths) {
  if (!tempo_edit_allowed()) return;

  if (tenths < (int16_t)kMinTempoTenths) tenths = kMinTempoTenths;
  if (tenths > (int16_t)kMaxTempoTenths) tenths = kMaxTempoTenths;
  tempo_tenths_ = (uint16_t)tenths;
  float bpm = (float)tempo_tenths_ / 10.0f;
  mcl_cfg.tempo = bpm;
  MidiClock.setTempo(bpm);
}

void TbdTempoPage::adjust_tempo(int16_t delta_tenths) {
  if (!tempo_edit_allowed()) return;

  set_tempo_tenths((int16_t)tempo_tenths_ + delta_tenths);
  tap_mode_ = false;
}

void TbdTempoPage::reset_taps() {
  tap_count_ = 0;
  tap_index_ = 0;
  last_tap_ms_ = 0;
  memset(tap_intervals_ms_, 0, sizeof(tap_intervals_ms_));
}

void TbdTempoPage::handle_tap() {
  if (!tempo_edit_allowed()) {
    tap_mode_ = false;
    reset_taps();
    return;
  }

  uint16_t now = read_clock_ms();
  if (tap_count_ == 0 ||
      clock_diff(last_tap_ms_, now) > 3000) {
    reset_taps();
    tap_count_ = 1;
    last_tap_ms_ = now;
    return;
  }

  uint16_t interval = clock_diff(last_tap_ms_, now);
  last_tap_ms_ = now;
  if (interval < 100) {
    return;
  }

  tap_intervals_ms_[tap_index_] = interval;
  tap_index_++;
  if (tap_index_ >= kMaxTapIntervals) {
    tap_index_ = 0;
  }
  if (tap_count_ < kMaxTapIntervals) {
    tap_count_++;
  }

  if (tap_count_ >= 4) {
    set_tempo_tenths(calculate_tap_tempo_tenths());
  }
}

uint16_t TbdTempoPage::calculate_tap_tempo_tenths() const {
  if (tap_count_ < 2) {
    return tempo_tenths_;
  }

  uint8_t count = tap_count_ - 1;
  if (count > kMaxTapIntervals) {
    count = kMaxTapIntervals;
  }

  uint32_t sum = 0;
  for (uint8_t i = 0; i < count; i++) {
    sum += tap_intervals_ms_[i];
  }
  if (sum == 0) {
    return tempo_tenths_;
  }

  uint32_t avg_ms = sum / count;
  if (avg_ms == 0) {
    return tempo_tenths_;
  }

  uint32_t tenths = 600000UL / avg_ms;
  if (tenths < kMinTempoTenths) tenths = kMinTempoTenths;
  if (tenths > kMaxTempoTenths) tenths = kMaxTempoTenths;
  return (uint16_t)tenths;
}

void TbdTempoPage::loop() {
  if (mcl.currentPage() != GRID_PAGE ||
      GUI.currentPage() != mcl.getPage(GRID_PAGE)) {
    close();
    return;
  }

  if (tempo_encoder_.hasChanged()) {
    int16_t delta = (int16_t)tempo_encoder_.cur;
    tempo_encoder_.cur = 0;
    tempo_encoder_.old = 0;
    if (delta != 0) {
      adjust_tempo(delta * 10);
    }
  }
}

bool TbdTempoPage::handleEvent(gui_event_t *event) {
  if (!EVENT_BUTTON(event)) {
    return false;
  }

  const bool is_press = event->mask == EVENT_BUTTON_PRESSED;
  const bool is_release = event->mask == EVENT_BUTTON_RELEASED;

  if (EVENT_PRESSED(event, ButtonsClass::BUTTON1)) {
    close();
    return true;
  }

  if (event->source == ButtonsClass::FUNC_BUTTON5) {
    if (is_press) {
      if (BUTTON_DOWN(ButtonsClass::TBD_BUTTON_B)) {
        tap_mode_ = true;
        handle_tap();
      } else if (BUTTON_DOWN(ButtonsClass::BUTTON3)) {
        tap_mode_ = true;
        handle_tap();
      } else {
        close();
      }
    }
    return true;
  }

  if (event->source == ButtonsClass::TBD_BUTTON_B) {
    if (BUTTON_DOWN(ButtonsClass::FUNC_BUTTON5) && is_press) {
      tap_mode_ = true;
      handle_tap();
    }
    return true;
  }

  if (event->source == ButtonsClass::BUTTON3) {
    if (BUTTON_DOWN(ButtonsClass::FUNC_BUTTON5)) {
      if (is_press) {
        tap_mode_ = true;
        handle_tap();
      }
    }
    return true;
  }

  if ((event->source == ButtonsClass::FUNC_BUTTON6 ||
       event->source == ButtonsClass::FUNC_BUTTON8) &&
      is_press) {
    adjust_tempo(event->source == ButtonsClass::FUNC_BUTTON6 ? 1 : -1);
    return true;
  }

  return false;
}

void TbdTempoPage::format_tempo(char *dst, size_t dst_len,
                                uint16_t tempo_tenths) const {
  snprintf(dst, dst_len, "%u.%u",
           (unsigned)(tempo_tenths / 10),
           (unsigned)(tempo_tenths % 10));
}

const char *TbdTempoPage::clock_source_label() const {
  switch (mcl_cfg.clock_rec) {
  case MIDI_CLOCK_SOURCE_INTERNAL:
    return "INT";
  case MIDI_CLOCK_SOURCE_PORT1:
    return "EXT1";
  case MIDI_CLOCK_SOURCE_PORT2:
    return "EXT2";
  case MIDI_CLOCK_SOURCE_USB:
    return "USB";
  default:
    return "---";
  }
}

void TbdTempoPage::draw_title(const char *title) {
  constexpr uint8_t kWinX = 16;
  constexpr uint8_t kWinY = 2;
  constexpr uint8_t kWinW = 96;

  oled_display.setFont();
  oled_display.setTextSize(1);
  oled_display.setTextColor(WHITE);
  oled_display.setCursor(kWinX + 4, kWinY + 3);
  oled_display.print(title);

  const char *source = clock_source_label();
  uint8_t source_w = (uint8_t)strlen(source) * 6;
  oled_display.setCursor(kWinX + kWinW - source_w - 4, kWinY + 3);
  oled_display.print(source);
}

void TbdTempoPage::draw_tempo_value(uint16_t tempo_tenths, uint8_t y) {
  char whole[5];
  snprintf(whole, sizeof(whole), "%u", (unsigned)(tempo_tenths / 10));
  uint8_t whole_w = 0;
  for (uint8_t i = 0; whole[i] != '\0'; i++) {
    whole_w += (whole[i] == '1') ? 4 : 7;
  }
  constexpr uint8_t frac_w = 12;
  uint8_t x = (128 - whole_w - frac_w) / 2;

  oled_display.setFont(&Elektrothic);
  oled_display.setTextSize(1);
  oled_display.setTextColor(WHITE);
  oled_display.setCursor(x, y);
  oled_display.print(whole);

  oled_display.setFont();
  oled_display.setTextSize(1);
  oled_display.setCursor(x + whole_w + 2, y - 7);
  oled_display.print('.');
  oled_display.print((unsigned)(tempo_tenths % 10));
}

void TbdTempoPage::display() {
  constexpr uint8_t kWinX = 16;
  constexpr uint8_t kWinY = 2;
  constexpr uint8_t kWinW = 96;
  constexpr uint8_t kWinH = 30;

  oled_display.fillRect(kWinX - 1, kWinY - 1, kWinW + 2, kWinH + 2, BLACK);
  oled_display.drawRect(kWinX, kWinY, kWinW, kWinH, WHITE);

  if (tap_mode_) {
    draw_title("TAP TEMPO");
    draw_tempo_value(tempo_tenths_, kWinY + 20);
    constexpr uint8_t box_w = 4;
    constexpr uint8_t spacing = 8;
    uint8_t x = 64 - 14;
    for (uint8_t i = 0; i < 4; i++) {
      uint8_t bx = x + i * spacing;
      if (i < tap_count_) {
        oled_display.fillRect(bx, kWinY + 23, box_w, box_w, WHITE);
      } else {
        oled_display.drawRect(bx, kWinY + 23, box_w, box_w, WHITE);
      }
    }
  } else {
    draw_title("TEMPO");
    draw_tempo_value(tempo_tenths_, kWinY + 22);
  }
}

#endif // PLATFORM_TBD
