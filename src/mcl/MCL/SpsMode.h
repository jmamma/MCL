/* TBD SPS-mode latch. Owns the FUNC_BUTTON5 toggle, the latched
 * arrow→BANKA-D / BUTTON1→BANKGROUP / trig→MD_GUI_TRIG_* routing, and
 * the FUNC + ARROW window-toggle chord. Pulled out of tbd_handleEvent
 * so the per-event ladder in MCL.cpp stays readable.
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

class SpsMode {
public:
  bool is_active() const { return latched_; }

  bool handle_toggle_button(gui_event_t *event);
  bool handle_func_arrow_chord(gui_event_t *event);
  bool handle_button1(gui_event_t *event);
  bool handle_arrow_to_bank(gui_event_t *event);
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
  uint8_t bound_page_ = 255;
  // Per-encoder "last used" timestamp for the value-show timeout (matches
  // LightPage::encoders_used_clock). Reset whenever cur changes; cleared
  // when the timeout has fully elapsed.
  uint16_t enc_used_clock_[4] = {0, 0, 0, 0};

  void set_latched(bool v);
  bool encoder_passthrough_page() const;
  void send_param(uint8_t i);
  bool show_value(uint8_t i) const;
};

extern SpsMode sps_mode;
