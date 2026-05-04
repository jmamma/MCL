#include "SpsMode.h"

#ifdef PLATFORM_TBD
#include "MCL.h"
#include "MD.h"
#include "MDParams.h"
#include "GridPage.h"
#include "GridPages.h"
#include "KeyInterface.h"
#include "GUI_hardware.h"
#include "MCLGUI.h"
#include "ResourceManager.h"
#include "BankPopupPage.h"

SpsMode sps_mode;

namespace {

inline bool is_arrow_source(uint8_t source) {
  return source == ButtonsClass::FUNC_BUTTON6 ||
         source == ButtonsClass::FUNC_BUTTON7 ||
         source == ButtonsClass::FUNC_BUTTON8 ||
         source == ButtonsClass::FUNC_BUTTON9;
}

inline bool is_press(const gui_event_t *event) {
  return event->mask == EVENT_BUTTON_PRESSED;
}

inline bool is_release(const gui_event_t *event) {
  return !(event->mask & 1);
}

} // namespace

bool SpsMode::encoder_passthrough_page() const {
  // While SPS is latched the kit-param encoders take precedence on
  // most pages, including the sequencer step / ptc / extstep pages
  // (the user expects to keep tweaking kit params while editing
  // patterns). Only pages that genuinely depend on the encoder
  // column for their core gesture (page select, bank popup) opt out.
  PageIndex pg = mcl.currentPage();
  return (pg == PAGE_SELECT_PAGE || pg == BANK_POPUP_PAGE);
}

bool SpsMode::display_passthrough_page() const {
  // Narrower than encoder_passthrough_page — only pages that genuinely
  // own the screen back off the SPS strip / overlay. Step / perf / ptc
  // pages keep their encoders (kit-param hijack stays disabled there)
  // but the SPS readout still draws so the user can see the active
  // page's params while editing steps.
  PageIndex pg = mcl.currentPage();
  return (pg == PAGE_SELECT_PAGE || pg == BANK_POPUP_PAGE);
}

void SpsMode::set_latched(bool v) {
  latched_ = v;
  GUI_hardware.led.sps_active = v;
  GUI_hardware.led.updateLeds = true;
  if (v) {
    resync_from_kit();
  } else if (mcl.currentPage() == BANK_POPUP_PAGE) {
    bank_popup_page.close();
  }
}

void SpsMode::resync_from_kit() {
  bound_track_ = MD.currentTrack;
  bound_sub_page_ = sub_page_;
  // MD.currentTrack should always be 0..15 but a stray track-index
  // reply has been observed as 24 — fall back to 0 rather than OOB.
  const uint8_t track = (MD.currentTrack < 16) ? MD.currentTrack : 0;
  uint8_t base = param_base();
  for (uint8_t i = 0; i < 4; i++) {
    uint8_t param = base + i;
    if (param >= MD_PARAMS_PER_TRACK) {
      enc[i].setValue(0);
      continue;
    }
    enc[i].setValue(MD.kit.params[track][param]);
  }
}

void SpsMode::send_param(uint8_t i) {
  if (!MD.connected) return;
  uint8_t param = param_base() + i;
  if (param >= MD_PARAMS_PER_TRACK) return;
  uint8_t v = (uint8_t)enc[i].cur;
  MD.setTrackParam(MD.currentTrack, param, v, nullptr, true);
}

bool SpsMode::handle_toggle_button(gui_event_t *event) {
  if (event->source != ButtonsClass::TBD_KEY_SPS_TOGGLE) return false;
  // TR semantics:
  //   tap (≤ TBD_TAP_MAX_MS, no chord)        → toggle SPS latch
  //   hold (≥ TBD_OVERLAY_HOLD_MS)            → open the param-select
  //                                             overlay (sticky; stays
  //                                             open after release).
  //                                             Forces latch on.
  //   release with overlay already open       → close overlay, latch
  //                                             unchanged.
  //   release between tap and overlay window  → no-op.
  // poll_page_overlay handles the hold→overlay timer transition; the
  // press/release branches here just gate the tap-toggle path and the
  // overlay-close-on-press behaviour.
  if (is_press(event)) {
    tr_consumed_ = false;
    tr_press_ms_ = read_clock_ms();
    tr_pressed_ = true;
  } else if (is_release(event)) {
    tr_pressed_ = false;
    const uint16_t held = clock_diff(tr_press_ms_, read_clock_ms());
    const bool was_tap = (held <= TBD_TAP_MAX_MS);
    if (overlay_open_) {
      // While the overlay is open, a tap closes it (no latch change).
      // A long hold (the gesture that opened it in the first place,
      // user still holding through release) leaves it open.
      if (was_tap) {
        overlay_open_ = false;
        GUI_hardware.led.sps_overlay = false;
        GUI_hardware.led.updateLeds = true;
        painted_sub_page_ = 255; // force LED repaint
      }
    } else if (was_tap && !tr_consumed_) {
      // Pure tap with no overlay and no chord — toggle the latch.
      set_latched(!latched_);
    }
    tr_consumed_ = false;
  }
  return true;
}

bool SpsMode::handle_func_arrow_chord(gui_event_t *event) {
  if (!is_arrow_source(event->source)) return false;
  if (!key_interface.is_key_down(MDX_KEY_FUNC)) return false;
  if (MD.connected && is_press(event)) {
    switch (event->source) {
      case ButtonsClass::FUNC_BUTTON6: MD.toggle_accent_window(); break; // UP
      case ButtonsClass::FUNC_BUTTON9: MD.toggle_swing_window();  break; // RIGHT
      case ButtonsClass::FUNC_BUTTON8: MD.toggle_slide_window();  break; // DOWN
      case ButtonsClass::FUNC_BUTTON7: MD.toggle_mute_window();   break; // LEFT
      default: break;
    }
  }
  return true;
}

bool SpsMode::handle_cluster_menus(gui_event_t *event) {
  if (!latched_) return false;
  // BUTTON1=A → MD SCALE (with FUNC variant for the scale-window
  // shortcut). BUTTON1=A also runs through the global MDX_KEY_NO
  // emission in tbd_handleEvent (which falls through in SPS-latched
  // so we can also fire SCALE here). BUTTON4=X is the global
  // MDX_KEY_YES passthrough; BUTTON3=Y is unused in SPS-latched.
  if (event->source != ButtonsClass::BUTTON1) return false;
  if (!MD.connected) return true;

  const bool func_held = key_interface.is_key_down(MDX_KEY_FUNC);
  // SCALE is the only key with both basic-key press/release and a
  // separate FUNC-shortcut (toggle_scale_window). Track whether we sent
  // hold_scale_button on the press edge so release pairs cleanly.
  static bool scale_held = false;

  if (is_press(event)) {
    if (func_held) {
      MD.toggle_scale_window();
    } else {
      MD.hold_scale_button();
      scale_held = true;
    }
  } else if (is_release(event)) {
    if (scale_held) {
      MD.release_scale_button();
      scale_held = false;
    }
  }
  return true;
}

bool SpsMode::handle_arrow_subpage(gui_event_t *event) {
  // Either MCL_B (TBD_KEY_SPS) or TR (TBD_KEY_SPS_TOGGLE) held + arrow
  // cycles sub_page_. Works with the latch off too — the gesture is
  // "modifier held", not "SPS-mode active". When TR is the modifier we
  // mark it consumed so the eventual TR release doesn't flip the latch.
  if (!is_arrow_source(event->source)) return false;
  const bool b_held  = BUTTON_DOWN(ButtonsClass::TBD_KEY_SPS);
  const bool tr_held = BUTTON_DOWN(ButtonsClass::TBD_KEY_SPS_TOGGLE);
  // While SPS is latched the cluster owns sub-page traversal — arrows
  // do page/half navigation regardless of modifier state. Outside the
  // latch, B-held or TR-held is still required so panel arrows fall
  // through to grid / seq navigation by default.
  if (!latched_ && !b_held && !tr_held) return false;
  if (is_press(event)) {
    // Suppress key-repeat: a held arrow only fires once per physical
    // press. The release branch clears arrow_consumed_source_ so the
    // next press of any arrow is honoured.
    if (arrow_consumed_source_ == event->source) return true;
    arrow_consumed_source_ = event->source;

    if (tr_held) tr_consumed_ = true;
    if (b_held)  b_consumed_  = true;
    // sub_page_ is the 4-param column id (0..7). The 8-param "page" is
    // sub_page_ >> 1, the half within the page is sub_page_ & 1.
    //   UP/DOWN  → flip half (toggle bit 0).
    //   LEFT/RIGHT → cycle page by ±2 columns, with wrap.
    // Stock MD firmware exposes 24 params (3 pages = 6 columns); SPS
    // firmware exposes 32 (4 pages = 8 columns). max_columns clips the
    // wrap range so we don't scroll into blank pages on a stock MD.
    const uint8_t max_columns = MD.is_spsx ? 8 : 6;
    if (sub_page_ >= max_columns) sub_page_ = max_columns - 1;

    switch (event->source) {
      case ButtonsClass::FUNC_BUTTON6: // UP
      case ButtonsClass::FUNC_BUTTON8: // DOWN
        sub_page_ ^= 1;
        if (sub_page_ >= max_columns) sub_page_ = max_columns - 1;
        break;
      case ButtonsClass::FUNC_BUTTON7: // LEFT
      case ButtonsClass::FUNC_BUTTON9: { // RIGHT
        const int8_t delta = (event->source == ButtonsClass::FUNC_BUTTON7) ? -2 : +2;
        const int next = (int)sub_page_ + delta;
        // Clamp at the edges — no wrap. Outside arrows become no-ops.
        if (next >= 0 && next < (int)max_columns) {
          sub_page_ = (uint8_t)next;
        }
        break;
      }
      default: break;
    }
    resync_from_kit();
  } else if (is_release(event)) {
    if (arrow_consumed_source_ == event->source) arrow_consumed_source_ = 255;
  }
  return true;
}

bool SpsMode::handle_sps_key_tap(gui_event_t *event) {
  if (event->source != ButtonsClass::TBD_KEY_SPS) return false;
  // B does NOT toggle the SPS latch — that's TR's role. Tap fires a
  // per-page action; hold (with arrow / trig chord) is the SPS sub-page
  // modifier (handle_arrow_subpage, handle_trig_forward). b_consumed_
  // suppresses the tap action when a chord ran during the hold.
  //   GRID_PAGE  → flip cur_grid (0/1 trig-grid view)
  //   SEQ_*      → advance sequencer page (BUTTON4 analog on AVR)
  if (is_press(event)) {
    b_consumed_ = false;
  } else if (is_release(event)) {
    if (!b_consumed_) {
      const PageIndex pg = mcl.currentPage();
      if (pg == GRID_PAGE) {
        // GridPage::handleEvent already implements the cur_grid swap
        // on MDX_KEY_SCALE press (resets param1.max, re-init). Post
        // that event so we get the full sequence — not just the bit
        // flip.
        key_interface.key_event(MDX_KEY_SCALE, false);
      } else if (pg == SEQ_STEP_PAGE || pg == SEQ_PTC_PAGE ||
                 pg == SEQ_EXTSTEP_PAGE) {
        // Synthesize the BUTTON4 release that SeqPage::handleEvent
        // already handles for page-select advance — avoids duplicating
        // the track-length / page-count clamp that lives there.
        gui_event_t e;
        e.type = BUTTON;
        e.source = ButtonsClass::BUTTON4;
        e.mask = EVENT_BUTTON_RELEASED;
        e.port = 0;
        GUI.putEvent(&e);
      }
    }
    b_consumed_ = false;
  }
  return true;
}

bool SpsMode::handle_trig_forward(gui_event_t *event, uint8_t trig_idx) {
  if (!latched_) return false;
  if (trig_idx >= 16) return false;

  // While SPS is latched the trig grid doubles as the sub-page
  // selector: each LED = 1 sub-page (4 params); top trig N → sub_page
  // 2N, bottom trig N+8 → sub_page 2N+1. Latch alone is sufficient
  // — B / TR held only mark themselves consumed (so the matching
  // release doesn't fire their tap action).
  // Pages that have a hard claim on trig (step toggle, bank popup,
  // page select) fall through unless the user explicitly chord-
  // overrides with B / TR.
  const bool b_held  = BUTTON_DOWN(ButtonsClass::TBD_KEY_SPS);
  const bool tr_held = BUTTON_DOWN(ButtonsClass::TBD_KEY_SPS_TOGGLE);
  PageIndex pg = mcl.currentPage();
  const bool page_owns_trig =
      (pg == SEQ_STEP_PAGE || pg == SEQ_PTC_PAGE ||
       pg == SEQ_EXTSTEP_PAGE || pg == PERF_PAGE_0 ||
       pg == BANK_POPUP_PAGE || pg == PAGE_SELECT_PAGE);
  if (page_owns_trig && !b_held && !tr_held) return false;

  if (is_press(event)) {
    const uint8_t max_sub_pages = MD.is_spsx ? 8 : 6;
    const uint8_t page_col = trig_idx & 0x07;
    const uint8_t half = (trig_idx >= 8) ? 1 : 0;
    const uint8_t target = page_col * 2 + half;
    if (target < max_sub_pages) {
      sub_page_ = target;
      resync_from_kit();
    }
    if (b_held)  b_consumed_  = true;
    if (tr_held) tr_consumed_ = true;
  }
  return true;
}

bool SpsMode::show_value(uint8_t i) const {
  // Visible only for SHOW_VALUE_TIMEOUT after the last rotation. Press
  // alone does NOT pop the value — pressing ENC1 is the PageSelect tap
  // gesture, and we don't want the strip to flash the value during it.
  if (enc_used_clock_[i] == 0) return false;
  return clock_diff(enc_used_clock_[i], read_clock_ms()) < SHOW_VALUE_TIMEOUT;
}

void SpsMode::poll_encoders() {
  if (!latched_) return;
  if (encoder_passthrough_page()) return;

  // Track / sub-page change: bring our cur values back in sync with the
  // kit so the encoder strip doesn't show a stale snapshot.
  if (bound_track_ != MD.currentTrack || bound_sub_page_ != sub_page_) {
    resync_from_kit();
  }

  // Snapshot the panel deltas, clear the source so the active page
  // doesn't also rotate, then run the standard MCLEncoder update path.
  encoder_t snapshot[4];
  for (uint8_t i = 0; i < 4; i++) {
    snapshot[i] = Encoders.encoders[i];
    Encoders.encoders[i].normal = 0;
  }
  uint16_t now = read_clock_ms();
  for (uint8_t i = 0; i < 4; i++) {
    enc[i].update(&snapshot[i]);
    if (enc[i].cur < enc[i].min) enc[i].cur = enc[i].min;
    if (enc[i].cur > enc[i].max) enc[i].cur = enc[i].max;
    if (enc[i].hasChanged()) {
      send_param(i);
      enc[i].old = enc[i].cur;
      enc_used_clock_[i] = now ? now : 1; // 0 reserved for "never"
    }
  }
}

void SpsMode::poll_page_overlay() {
  // TR-hold edge: when held past TBD_OVERLAY_HOLD_MS, open the overlay
  // (sticky) and force the latch on. Driven by the event-set
  // tr_pressed_ flag rather than BUTTON_DOWN — the latter races with
  // the panel poll and would fire on stale state with a stale
  // tr_press_ms_.
  if (tr_pressed_ && !overlay_open_ &&
      clock_diff(tr_press_ms_, read_clock_ms()) > TBD_OVERLAY_HOLD_MS) {
    overlay_open_ = true;
    if (!latched_) set_latched(true);
    GUI_hardware.led.sps_overlay = true;
    GUI_hardware.led.updateLeds = true;
  }

  // Both the OLED overlay and the LED column-blink follow overlay_open_
  // (sticky). The LEDs back off on display-passthrough pages so we
  // don't fight a page that owns the panel palette.
  const bool show = overlay_open_ && latched_ && !display_passthrough_page();
  if (!show) {
    if (painted_sub_page_ != 255) {
      mcl_gui.reset_trigleds();
      painted_sub_page_ = 255;
    }
    return;
  }
  if (painted_sub_page_ == sub_page_) return;

  // LED model — 1 trig LED = 1 sub-page (4 params); a column-pair
  // (top trig N + bottom trig N+8) is one 8-param page. Stock MD =
  // 3 pages; SPS firmware = 4. All available pairs light standard red
  // (matches sequencer trig palette); the focused sub-page (the one
  // the encoders edit) is solid white. No blinking.
  constexpr uint32_t kRed   = ((uint32_t)255 << 16);
  constexpr uint32_t kWhite = ((uint32_t)255 << 16) |
                              ((uint32_t)255 << 8)  |
                              (uint32_t)255;

  const uint8_t max_pages = MD.is_spsx ? 4 : 3;

  // Clear unavailable pairs so they don't leak from a prior paint.
  for (uint8_t page = max_pages; page < 8; page++) {
    const uint16_t bits = ((uint16_t)1 << page) | ((uint16_t)1 << (page + 8));
    mcl_gui.set_trigleds_color(bits, 0);
  }

  // Available pairs: dim red.
  uint16_t avail = 0;
  for (uint8_t page = 0; page < max_pages; page++) {
    avail |= ((uint16_t)1 << page) | ((uint16_t)1 << (page + 8));
  }
  mcl_gui.set_trigleds_color(avail, kRed);

  // Focused sub-page: white. sub_page_ is the column id; bit position
  // is `sub_page_ >> 1` for the page column, `& 1` chooses top/bottom.
  const uint8_t focus_page = sub_page_ >> 1;
  const uint8_t focus_half = sub_page_ & 1;
  const uint16_t focus_bit =
      (focus_half == 0) ? ((uint16_t)1 << focus_page)
                        : ((uint16_t)1 << (focus_page + 8));
  mcl_gui.set_trigleds_color(focus_bit, kWhite);

  painted_sub_page_ = sub_page_;
}

void SpsMode::draw_overlay() {
  // Sticky — overlay_open_ is set by poll_page_overlay once TR has
  // been held past TBD_OVERLAY_HOLD_MS, and cleared by a TR tap that
  // closes it. Doesn't depend on TR being held while drawing.
  if (!latched_) return;
  if (!overlay_open_) return;
  if (display_passthrough_page()) return;

  // Both halves render: top = params 0..3 of the page, bottom = 4..7.
  // active_half (toggled by UP/DOWN) selects which half is bound to
  // the encoders — marked by a 1-px bounding rect around its 128x32
  // region so the user can see what the encoders will edit.
  const uint8_t page = sub_page_ >> 1;       // 0..3
  const uint8_t active_half = sub_page_ & 1; // 0 = top, 1 = bottom
  const uint8_t page_base = page * 8;
  // Defensive: clamp track to the kit's 0..15 range. Stray track-index
  // replies from the MD have been seen as 24, which would OOB-read the
  // 16-row params array.
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

  // Page-nav indicators: filled triangles in the gutter between the
  // two strips, far-left and far-right. Only shown when LEFT/RIGHT
  // would actually move (no wrap).
  const uint8_t max_columns = MD.is_spsx ? 8 : 6;
  const bool can_left  = ((int)sub_page_ - 2) >= 0;
  const bool can_right = ((int)sub_page_ + 2) < (int)max_columns;
  if (can_left) {
    oled_display.fillRect(0, 28, 2, 7, BLACK);
    oled_display.fillTriangle(6, 28, 6, 34, 2, 31, WHITE);
  }
  if (can_right) {
    oled_display.fillRect(126, 28, 2, 7, BLACK);
    oled_display.fillTriangle(121, 28, 121, 34, 125, 31, WHITE);
  }
}

void SpsMode::draw_strip(uint8_t y_top) {
  if (!latched_) return;
  if (display_passthrough_page()) return;

  Encoder *encs[4];
  const char *labels[4];
  bool show[4];
  uint8_t base = param_base();
  uint8_t model = MD.kit.get_model(MD.currentTrack);
  for (uint8_t i = 0; i < 4; i++) {
    uint8_t param = base + i;
    encs[i] = (param < MD_PARAMS_PER_TRACK) ? &enc[i] : nullptr;
    labels[i] = (param < MD_PARAMS_PER_TRACK)
                    ? model_param_name(model, param)
                    : nullptr;
    show[i] = encs[i] ? show_value(i) : false;
  }
  mcl_gui.draw_encoder_strip(y_top, encs, labels, show);
}

#else // !PLATFORM_TBD

SpsMode sps_mode;

bool SpsMode::handle_toggle_button(gui_event_t *) { return false; }
bool SpsMode::handle_func_arrow_chord(gui_event_t *) { return false; }
bool SpsMode::handle_cluster_menus(gui_event_t *) { return false; }
bool SpsMode::handle_arrow_subpage(gui_event_t *) { return false; }
bool SpsMode::handle_sps_key_tap(gui_event_t *) { return false; }
bool SpsMode::handle_trig_forward(gui_event_t *, uint8_t) { return false; }
void SpsMode::poll_encoders() {}
void SpsMode::draw_strip(uint8_t) {}
void SpsMode::poll_page_overlay() {}
void SpsMode::draw_overlay() {}
void SpsMode::resync_from_kit() {}
void SpsMode::send_param(uint8_t) {}
bool SpsMode::encoder_passthrough_page() const { return true; }
bool SpsMode::display_passthrough_page() const { return true; }
void SpsMode::set_latched(bool) {}

#endif // PLATFORM_TBD
