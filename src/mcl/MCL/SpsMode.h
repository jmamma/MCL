/* TBD SPS-mode latch. Owns the TBD_KEY_SPS_TOGGLE (TOP_RIGHT) toggle,
 * the cluster Y/X/A → MD menu-open 0x40 sysex (scale / LFO / kit), the
 * trig → sub_page selector / MD_GUI_TRIG_* routing, the SPS-key + arrow
 * sub-page traversal, and the FUNC + ARROW window-toggle chord. Pulled
 * out of tbd_handleEvent so the per-event ladder in MCL.cpp stays
 * readable.
 *
 * In latched state, the four hardware encoders are also captured and
 * routed to live MD kit-param updates for the current synth page —
 * implemented as proper MCLEncoder instances (so rate-limiting and
 * fast-mode behave like every other page-bound encoder), and drawn
 * in the bottom 32 px of the OLED on TBD.
 *
 * TBD-only — guarded by PLATFORM_TBD at the include site. The header
 * still compiles on non-TBD targets but the helpers are no-ops.
 */

#pragma once

#include "GUI.h"
#include "MCLEncoder.h"

// Standard tap window for TBD-panel gestures: encoder taps (ENC1
// pattern-select, ENC2/3/4 cluster taps), B latch toggle vs.
// modifier hold, etc. Anything held longer than this counts as
// a sustained press / hold gesture.
#define TBD_TAP_MAX_MS 200

// TR-hold threshold for engaging the SPS latch. Below this a TR press
// is a no-op; past it set_latched(true) fires (one-shot per press).
#define TBD_LATCH_HOLD_MS 250

// TR-hold threshold for the SPS param-select overlay. Below this the
// hold has either done nothing (< TBD_LATCH_HOLD_MS) or just engaged
// the latch; past it the overlay + column LEDs render.
#define TBD_OVERLAY_HOLD_MS 500

class SpsMode {
public:
  bool is_active() const { return latched_; }

  bool handle_toggle_button(gui_event_t *event);
  // Suppress the next TR-release latch toggle. Called by chord handlers
  // (TL→TR system-page, TR+arrow sub-page traversal) so a held-TR used
  // as a modifier doesn't also flip the latch on release.
  void mark_tr_consumed() { tr_consumed_ = true; }
  // Same convention as mark_tr_consumed but for B. Called when B is
  // used as a modifier (B+arrow sub-page traversal, B+trig column
  // selector) so the matching B release doesn't toggle the latch.
  void mark_b_consumed() { b_consumed_ = true; }
  bool handle_func_arrow_chord(gui_event_t *event);
  // Cluster Y/X/A in SPS-latched: Y → MD NO transmit, X → MD YES,
  // A → MD SCALE (FUNC variant: toggle_scale_window).
  bool handle_cluster_menus(gui_event_t *event);
  // MCL_B held + arrow: cycle sub_page_ and resync the encoder strip.
  // Takes precedence over the FUNC+arrow window-toggle.
  bool handle_arrow_subpage(gui_event_t *event);
  // MCL_B (TBD_KEY_SPS) on press while latched: FUNC variant toggles the
  // MD LFO menu; bare variant fires a single press_page_button (no paired
  // release — the MD's PAGE handler runs on both edges, so one sysex
  // produces one page advance).
  bool handle_sps_key_tap(gui_event_t *event);
  bool handle_trig_forward(gui_event_t *event, uint8_t trig_idx);

  // Drives the four kit-param encoders. Call from MCL::loop *before*
  // GUI.loop() so the panel deltas don't also reach the active page.
  // No-op when the latch is off or when the active page already owns
  // the encoder column (step / perf / page-select).
  void poll_encoders();

  // Draw the encoder strip on the bottom of the OLED. Call from
  // GridPage::display() (or any other 32-px-tall page that wants to
  // show it). No-op when the latch is off.
  void draw_strip(uint8_t y_top);

  // Paint the trig LEDs to show the active sub-page: each available
  // sub-page lights its column dimly; the active one's column blinks.
  // Call from MCL::loop() so it tracks sub_page_ changes.
  void poll_page_overlay();

  // Full-screen overlay drawn while MCL_B (TBD_KEY_SPS) is held + the
  // latch is on: top half shows params 0..3 of the current 8-param
  // page, bottom half shows 4..7. The 4 encoders are bound to whichever
  // half sub_page_ points at; the other half renders read-only values
  // pulled from MD.kit.params. No-op otherwise. Hooked from
  // GuiClass::display() after the active page draws.
  void draw_overlay();

  // Sync the encoder cur values to MD.kit.params for the currently
  // active track + synth page. Call when latching on, when the user
  // changes track / synth page, or when an inbound CC / kit-load may
  // have moved the underlying values.
  void resync_from_kit();

  // The 4 encoders themselves. min=0, max=127, default rot_res — same
  // feel as a stock MCLEncoder.
  MCLEncoder enc[4] = {
      MCLEncoder(127, 0, 1, 4),
      MCLEncoder(127, 0, 1, 4),
      MCLEncoder(127, 0, 1, 4),
      MCLEncoder(127, 0, 1, 4),
  };

private:
  bool latched_ = false;
  uint8_t bound_track_ = 255;
  uint8_t bound_sub_page_ = 255;
  // Param window selector (0..7). Each window covers 4 consecutive
  // params; SPS-key + arrow cycles. With legacy (24-param) kits only
  // 0..5 are populated; SPSX (34) extends to 8.
  uint8_t sub_page_ = 0;
  // Per-encoder "last used" timestamp for the value-show timeout (matches
  // LightPage::encoders_used_clock). Reset whenever cur changes; cleared
  // when the timeout has fully elapsed.
  uint16_t enc_used_clock_[4] = {0, 0, 0, 0};
  // TR-press lifetime flag: set when a chord or arrow used TR as a
  // modifier; cleared on the next TR press edge. Suppresses the
  // tap-toggle on TR release.
  bool tr_consumed_ = false;
  // Timestamp of the most recent TR press edge. The release uses
  // (now - tr_press_ms_) to distinguish a tap (≤ tap-threshold) from
  // a hold (the overlay-view gesture) so a held TR doesn't unlatch
  // SPS when the user just wanted to peek at the params.
  uint16_t tr_press_ms_ = 0;
  // Event-driven TR-held flag — set true on TR press, cleared on
  // release. Used by poll_page_overlay's hold-timer instead of
  // BUTTON_DOWN, which can race with the panel poll and read stale
  // values (causing the overlay to appear immediately when
  // tr_press_ms_ is still 0/old).
  bool tr_pressed_ = false;
  // B-press lifetime flag: equivalent to tr_consumed_ but for the
  // latch-toggle role that now lives on B. Cleared on B press; if no
  // chord set it during the hold, B release toggles the latch.
  bool b_consumed_ = false;
  // Last sub_page we painted; 255 = nothing painted (force a repaint
  // when latching back on or returning from a passthrough page).
  uint8_t painted_sub_page_ = 255;
  // Sticky open-state for the param-page select overlay. Set when TR
  // is held past TBD_OVERLAY_HOLD_MS; cleared by the next TR press
  // (release closes the overlay without toggling the latch).
  bool overlay_open_ = false;
  // Source of the arrow press currently being held in B-overlay mode.
  // Used to suppress repeat-press events on the same physical hold so
  // the sub-page selector doesn't flip on every event the panel emits.
  // Cleared on release of any arrow.
  uint8_t arrow_consumed_source_ = 255;
  void set_latched(bool v);
  bool encoder_passthrough_page() const;
  bool display_passthrough_page() const;
  void send_param(uint8_t i);
  bool show_value(uint8_t i) const;
  uint8_t param_base() const { return (uint8_t)(sub_page_ * 4); }
};

extern SpsMode sps_mode;
