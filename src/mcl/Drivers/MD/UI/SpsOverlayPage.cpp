#include "SpsOverlayPage.h"

#ifdef PLATFORM_TBD

#include "MCL.h"
#include "MCLGUI.h"
#include "../MD.h"
#include "../MDParams.h"
#include "GUI_hardware.h"
#include "SpsMode.h"
#include <string.h>

namespace {
// XOR-invert a rectangle in the OLED framebuffer. Used to mark
// locked encoder slots (matches SpsMode::draw_strip's invert path).
static void invert_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
  for (uint8_t yy = y; yy < y + h; yy++) {
    for (uint8_t xx = x; xx < x + w; xx++) {
      oled_display.drawPixel(xx, yy, 2);
    }
  }
}
} // namespace

SpsOverlayPage sps_overlay_page;

void SpsOverlayPage::init() {
  GUI_hardware.led.sps_overlay = true;
  GUI_hardware.led.updateLeds = true;
  painted_sub_page_ = 255;
  // LEDs only paint while TR is currently held — see loop().
}

void SpsOverlayPage::cleanup() {
  GUI_hardware.led.sps_overlay = false;
  GUI_hardware.led.updateLeds = true;
  if (painted_sub_page_ != 255) {
    mcl_gui.reset_trigleds();
    painted_sub_page_ = 255;
  }
}

void SpsOverlayPage::loop() {
  // LED column palette is gated on TR being currently held — that's
  // the user's "I'm actively driving sub-page selection" gesture.
  // When TR is released, hand the LEDs back to the active page so
  // step edit / grid / etc. can repaint their own palette.
  const bool tr_held = BUTTON_DOWN(ButtonsClass::TBD_BUTTON_TR);
  if (tr_held) {
    paint_leds();
  } else if (painted_sub_page_ != 255) {
    mcl_gui.reset_trigleds();
    painted_sub_page_ = 255;
  }
}

void SpsOverlayPage::paint_leds() {
  if (painted_sub_page_ == MD.sps_mode.sub_page()) return;

  // 1 trig LED = 1 sub-page (4 params); a column-pair (top trig N +
  // bottom trig N+8) = one 8-param page. Stock MD = 3 pages, SPS = 4.
  // All available pairs light standard red; the focused sub-page is
  // solid white.
  constexpr uint32_t kRed   = ((uint32_t)255 << 16);
  constexpr uint32_t kWhite = ((uint32_t)255 << 16) |
                              ((uint32_t)255 << 8)  |
                              (uint32_t)255;

  const uint8_t max_pages = MD.is_spsx ? 4 : 3;

  // Clear unavailable pairs so they don't leak.
  for (uint8_t page = max_pages; page < 8; page++) {
    const uint16_t bits = ((uint16_t)1 << page) | ((uint16_t)1 << (page + 8));
    mcl_gui.set_trigleds_color(bits, 0);
  }

  uint16_t avail = 0;
  for (uint8_t page = 0; page < max_pages; page++) {
    avail |= ((uint16_t)1 << page) | ((uint16_t)1 << (page + 8));
  }
  mcl_gui.set_trigleds_color(avail, kRed);

  const uint8_t sub_page = MD.sps_mode.sub_page();
  const uint8_t focus_page = sub_page >> 1;
  const uint8_t focus_half = sub_page & 1;
  const uint16_t focus_bit =
      (focus_half == 0) ? ((uint16_t)1 << focus_page)
                        : ((uint16_t)1 << (focus_page + 8));
  mcl_gui.set_trigleds_color(focus_bit, kWhite);

  painted_sub_page_ = sub_page;
}

void SpsOverlayPage::display() {
  // Both halves render: top = params 0..3 of the page, bottom = 4..7.
  // active_half (toggled by UP/DOWN) selects which half is bound to
  // the encoders — marked by a 1-px bounding rect around its 128x32
  // region so the user can see what the encoders will edit.
  const uint8_t sub_page = MD.sps_mode.sub_page();
  const uint8_t page = sub_page >> 1;
  const uint8_t active_half = sub_page & 1;
  const uint8_t page_base = page * 8;
  // Defensive: clamp track to the kit's 0..15 range.
  const uint8_t track = (MD.currentTrack < 16) ? MD.currentTrack : 0;
  const uint8_t model = MD.kit.get_model(track);

  // Per-slot lock substitution: when on SEQ_STEP_PAGE with a trig
  // held, each slot's dial / value reflect the lock value (not the
  // kit value), and the value-text rect gets XOR-inverted as a flag.
  Encoder top_encs[4], bottom_encs[4];
  bool top_locked[4] = {false, false, false, false};
  bool bot_locked[4] = {false, false, false, false};
  for (uint8_t i = 0; i < 4; i++) {
    const uint8_t tp = page_base + i;
    const uint8_t bp = page_base + 4 + i;
    uint8_t lv;
    if (MD.sps_mode.active_step_lock(tp, &lv)) {
      top_encs[i].cur = lv;
      top_locked[i] = true;
    } else {
      top_encs[i].cur = MD.kit.params[track][tp];
    }
    top_encs[i].old = top_encs[i].cur;
    if (MD.sps_mode.active_step_lock(bp, &lv)) {
      bottom_encs[i].cur = lv;
      bot_locked[i] = true;
    } else {
      bottom_encs[i].cur = MD.kit.params[track][bp];
    }
    bottom_encs[i].old = bottom_encs[i].cur;
  }

  Encoder *top_strip[4]    = {&top_encs[0], &top_encs[1], &top_encs[2], &top_encs[3]};
  Encoder *bottom_strip[4] = {&bottom_encs[0], &bottom_encs[1], &bottom_encs[2], &bottom_encs[3]};
  const char *top_labels[4];
  const char *bottom_labels[4];
  bool top_show[4]    = {false, false, false, false};
  bool bottom_show[4] = {false, false, false, false};
  for (uint8_t i = 0; i < 4; i++) {
    const uint8_t tp = page_base + i;
    const uint8_t bp = page_base + 4 + i;
    top_labels[i]    = (tp < MD_PARAMS_PER_TRACK) ? model_param_name(model, tp) : nullptr;
    bottom_labels[i] = (bp < MD_PARAMS_PER_TRACK) ? model_param_name(model, bp) : nullptr;
    // Always show the value text on locked slots so the user sees
    // the lock value clearly.
    top_show[i]    = top_locked[i];
    bottom_show[i] = bot_locked[i];
  }

  mcl_gui.draw_encoder_strip(0,  top_strip,    top_labels,    top_show);
  mcl_gui.draw_encoder_strip(32, bottom_strip, bottom_labels, bottom_show);

  // Invert covering both dial + value text on locked slots.
  constexpr uint8_t kCellW = 32;
  constexpr uint8_t kDialW = 11;
  for (uint8_t i = 0; i < 4; i++) {
    if (top_locked[i]) {
      char val[4];
      mcl_gui.put_value_at((uint8_t)top_encs[i].cur, val);
      uint8_t vw = (uint8_t)strlen(val) * 6;
      uint8_t inner_w = (vw > kDialW) ? vw : kDialW;
      uint8_t cx = i * kCellW;
      uint8_t inv_x = cx + (kCellW - inner_w) / 2 - 2;
      invert_rect(inv_x, 0 + 3, inner_w + 4, 23);
    }
    if (bot_locked[i]) {
      char val[4];
      mcl_gui.put_value_at((uint8_t)bottom_encs[i].cur, val);
      uint8_t vw = (uint8_t)strlen(val) * 6;
      uint8_t inner_w = (vw > kDialW) ? vw : kDialW;
      uint8_t cx = i * kCellW;
      uint8_t inv_x = cx + (kCellW - inner_w) / 2 - 2;
      invert_rect(inv_x, 32 + 3, inner_w + 4, 23);
    }
  }

  // 1-px bounding box around the encoder-bound half.
  const uint8_t box_y = active_half * 32;
  oled_display.drawRect(0, box_y, 128, 32, WHITE);

  // Page-nav indicators: filled triangles in the gutter, far-left /
  // far-right. Only shown when LEFT/RIGHT would actually move (no wrap).
  const uint8_t max_columns = MD.is_spsx ? 8 : 6;
  const bool can_left  = ((int)sub_page - 2) >= 0;
  const bool can_right = ((int)sub_page + 2) < (int)max_columns;
  if (can_left) {
    oled_display.fillRect(0, 28, 2, 7, BLACK);
    oled_display.fillTriangle(6, 28, 6, 34, 2, 31, WHITE);
  }
  if (can_right) {
    oled_display.fillRect(126, 28, 2, 7, BLACK);
    oled_display.fillTriangle(121, 28, 121, 34, 125, 31, WHITE);
  }
}

#endif // PLATFORM_TBD
