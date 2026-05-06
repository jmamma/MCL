/* TBD SPS-mode latch. Owns the cluster A/X/Y MD key routing,
 * the trig → sub_page selector /
 * MD_GUI_TRIG_* routing, the SPS-key + arrow sub-page traversal, and
 * the FUNC + ARROW window-toggle chord. Pulled out of tbd_handleEvent
 * so the per-event ladder in MCL.cpp stays readable.
 *
 * In latched state, the four hardware encoders are also captured and
 * routed to live MD kit-param updates for the current synth page —
 * implemented as proper MCLEncoder instances (so rate-limiting and
 * fast-mode behave like every other page-bound encoder), and drawn
 * in the bottom 32 px of the OLED on TBD.
 *
 * The param-page-select overlay is now a real LightPage
 * (SpsOverlayPage). SpsMode pushes/pops it; rendering and event
 * handling for the overlay live on the page itself.
 *
 * TBD-only — guarded by PLATFORM_TBD at the include site. The header
 * still compiles on non-TBD targets but the helpers are no-ops.
 */

#pragma once

#include "GUI.h"
#include "MCLEncoder.h"

// TR-hold threshold for the SPS param-select overlay. Below this a
// TR press is just the latch-tap window; past it SpsOverlayPage gets
// pushed onto the page stack.
#define TBD_OVERLAY_HOLD_MS 500

class SpsMode {
public:
  bool is_active() const { return latched_; }
  uint8_t sub_page() const { return sub_page_; }
  void enable() { set_latched(true); }
  void disable() { set_latched(false); }

  bool handle_func_arrow_chord(gui_event_t *event);
  // Cluster X/Y in SPS-latched: X → MD EXTENDED / FUNC+EXTENDED,
  // Y → MD NO transmit.
  bool handle_cluster_menus(gui_event_t *event);
  // Arrow handler: latched → cycle sub_page_; SPS-key/TR-held → also cycle
  // (works without latch). UP/DOWN flip half within page; LEFT/RIGHT
  // step pages (clamped, no wrap).
  bool handle_arrow_subpage(gui_event_t *event);
  // Tracks whether the SPS key stayed a clean tap. If another button is
  // pressed while it is down, release is ignored instead of firing the
  // tap action.
  void observe_sps_key_chord(gui_event_t *event);
  // Physical FUNC tap fires a per-page action (grid swap on
  // GRID_PAGE; sequencer-page advance on SEQ_*). Any other button
  // pressed during the hold suppresses the release tap action.
  bool handle_sps_key_tap(gui_event_t *event);
  bool handle_trig_forward(gui_event_t *event, uint8_t trig_idx);

  // Drives the four kit-param encoders. Call from MCL::loop *before*
  // GUI.loop() so the panel deltas don't also reach the active page.
  // No-op when the latch is off or when the active page already owns
  // the encoder column (step / perf / page-select).
  void poll_encoders();

  // Polled from MCL::loop. Single responsibility: when TR has been
  // held past TBD_OVERLAY_HOLD_MS while latched, push SpsOverlayPage.
  // No rendering — the page owns its own LEDs / OLED via its
  // init / cleanup / display lifecycle.
  void poll_page_overlay();

  // Sync the encoder cur values to MD.kit.params for the currently
  // active track + synth page. Call when latching on, when the user
  // changes track / synth page, or when an inbound CC / kit-load may
  // have moved the underlying values.
  void resync_from_kit();

  // True if the user is on SEQ_STEP_PAGE with at least one trig held
  // and the active MD track's seq has a lock for `param` at the held
  // step. Out-param `value` receives the lock value (0..127). The
  // first held step is used when multiple are held.
  bool active_step_lock(uint8_t param, uint8_t *value) const;

  // Public accessor for the value-show timeout state (SpsStripPage
  // uses this to gate value-vs-label display per slot).
  bool show_strip_value(uint8_t i) const { return show_value(i); }

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
  // Timestamp of the most recent TR press edge.
  uint16_t tr_press_ms_ = 0;
  // Event-driven TR-held flag — set true on TR press, cleared on
  // release. Used by poll_page_overlay's hold-timer instead of
  // BUTTON_DOWN, which can race with the panel poll.
  bool tr_pressed_ = false;
  // X press emits EXTENDED or FUNC+EXTENDED depending on the modifier
  // state at press time; release must pair with the same key.
  uint8_t x_key_held_ = 255;
  // SPS-key press lifetime flag: cleared on press; if no chord set it
  // during the hold, release fires the per-page tap action.
  bool sps_key_consumed_ = false;
  // Source of the arrow press currently being held in SPS-key overlay mode.
  // Used to suppress repeat-press events on the same physical hold.
  uint8_t arrow_consumed_source_ = 255;
  void set_latched(bool v);
  bool encoder_passthrough_page() const;
  void send_param(uint8_t i);
  bool show_value(uint8_t i) const;
  uint8_t param_base() const { return (uint8_t)(sub_page_ * 4); }
};
