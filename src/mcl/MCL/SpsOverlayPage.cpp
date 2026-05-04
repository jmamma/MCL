#include "SpsOverlayPage.h"

#ifdef PLATFORM_TBD

#include "MCL.h"
#include "MCLGUI.h"
#include "MD.h"
#include "MDParams.h"
#include "GUI_hardware.h"
#include "KeyInterface.h"
#include "SpsMode.h"

SpsOverlayPage sps_overlay_page;

namespace {

inline bool is_press(const gui_event_t *event) {
  return event->mask == EVENT_BUTTON_PRESSED;
}

inline bool is_release(const gui_event_t *event) {
  return !(event->mask & 1);
}

inline bool is_arrow_source(uint8_t source) {
  return source == ButtonsClass::FUNC_BUTTON6 ||
         source == ButtonsClass::FUNC_BUTTON7 ||
         source == ButtonsClass::FUNC_BUTTON8 ||
         source == ButtonsClass::FUNC_BUTTON9;
}

} // namespace

void SpsOverlayPage::setup() {}

void SpsOverlayPage::init() {
  // Stash where we came from so cleanup / TR-tap-close can return.
  PageIndex cur_idx = mcl.currentPage();
  if (cur_idx != SPS_OVERLAY_PAGE) {
    prev_page_ = cur_idx;
  }

  GUI_hardware.led.sps_overlay = true;
  GUI_hardware.led.updateLeds = true;

  painted_sub_page_ = 255;
  paint_leds();
}

void SpsOverlayPage::cleanup() {
  GUI_hardware.led.sps_overlay = false;
  GUI_hardware.led.updateLeds = true;
  mcl_gui.reset_trigleds();
  painted_sub_page_ = 255;
}

void SpsOverlayPage::close() {
  if (prev_page_ != NULL_PAGE && prev_page_ != SPS_OVERLAY_PAGE) {
    mcl.setPage(prev_page_);
  } else {
    mcl.setPage(GRID_PAGE);
  }
}

void SpsOverlayPage::loop() {
  // Repaint LEDs if sub_page_ changed (e.g., user pressed an arrow or
  // a trig and SpsMode bumped sub_page_).
  paint_leds();
}

void SpsOverlayPage::paint_leds() {
  if (painted_sub_page_ == sps_mode.sub_page()) return;

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

  const uint8_t sub_page = sps_mode.sub_page();
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
  const uint8_t sub_page = sps_mode.sub_page();
  const uint8_t page = sub_page >> 1;
  const uint8_t active_half = sub_page & 1;
  const uint8_t page_base = page * 8;
  // Defensive: clamp track to the kit's 0..15 range.
  const uint8_t track = (MD.currentTrack < 16) ? MD.currentTrack : 0;
  const uint8_t model = MD.kit.get_model(track);

  Encoder top_encs[4], bottom_encs[4];
  for (uint8_t i = 0; i < 4; i++) {
    top_encs[i].cur    = MD.kit.params[track][page_base + i];
    top_encs[i].old    = top_encs[i].cur;
    bottom_encs[i].cur = MD.kit.params[track][page_base + 4 + i];
    bottom_encs[i].old = bottom_encs[i].cur;
  }

  Encoder *top_strip[4]    = {&top_encs[0], &top_encs[1], &top_encs[2], &top_encs[3]};
  Encoder *bottom_strip[4] = {&bottom_encs[0], &bottom_encs[1], &bottom_encs[2], &bottom_encs[3]};
  const char *top_labels[4];
  const char *bottom_labels[4];
  bool show_off[4] = {false, false, false, false};
  for (uint8_t i = 0; i < 4; i++) {
    const uint8_t tp = page_base + i;
    const uint8_t bp = page_base + 4 + i;
    top_labels[i]    = (tp < MD_PARAMS_PER_TRACK) ? model_param_name(model, tp) : nullptr;
    bottom_labels[i] = (bp < MD_PARAMS_PER_TRACK) ? model_param_name(model, bp) : nullptr;
  }

  mcl_gui.draw_encoder_strip(0,  top_strip,    top_labels,    show_off);
  mcl_gui.draw_encoder_strip(32, bottom_strip, bottom_labels, show_off);

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

bool SpsOverlayPage::handleEvent(gui_event_t *event) {
  // Arrow + trig events for sub-page navigation. SpsMode's existing
  // handlers do the work — they're page-agnostic; we just call them
  // and consume the event.
  if (sps_mode.handle_arrow_subpage(event)) return true;

  if (EVENT_BUTTON(event)) {
    const uint8_t src = event->source;

    // TR tap closes the overlay (no latch change). Long hold release
    // is ignored — it's the gesture that opened us.
    if (src == ButtonsClass::TBD_KEY_SPS_TOGGLE) {
      // Forward to SpsMode so it can manage tr_press_ms_ and
      // arrow-modifier consumption. SpsMode's handler returns true
      // for a tap-close; we pop on that.
      bool was_tap_close = sps_mode.handle_overlay_tr_event(event);
      if (was_tap_close) close();
      return true;
    }

    // B-tap also closes, then performs the per-page action on the
    // page underneath (grid swap / seq page advance).
    if (src == ButtonsClass::TBD_KEY_SPS) {
      if (is_release(event)) {
        close();
        // Re-dispatch the B release through SpsMode so its per-page
        // tap action fires on the now-current underlying page.
        sps_mode.handle_sps_key_tap(event);
      }
      return true;
    }

    // Trig 0..15 → sub-page jump via SpsMode's existing logic.
    if (src >= ButtonsClass::TRIG_BUTTON1 &&
        src < ButtonsClass::TRIG_BUTTON1 + 16) {
      const uint8_t trig_idx = src - ButtonsClass::TRIG_BUTTON1;
      sps_mode.handle_trig_forward(event, trig_idx);
      return true;
    }
  }
  return false;
}

#endif // PLATFORM_TBD
