/* TBD SPS-mode latch. Owns the FUNC_BUTTON5 toggle, the latched
 * arrow→BANKA-D / BUTTON1→BANKGROUP / trig→MD_GUI_TRIG_* routing, and
 * the FUNC + ARROW window-toggle chord. Pulled out of tbd_handleEvent
 * so the per-event ladder in MCL.cpp stays readable.
 *
 * TBD-only — guarded by PLATFORM_TBD at the include site. The header
 * still compiles on non-TBD targets but the helpers are no-ops.
 */

#pragma once

#include "GUI.h"

class SpsMode {
public:
  bool is_active() const { return latched_; }

  // FUNC_BUTTON5 (panel button next to STOP) toggles SPS mode. Eats the
  // event so it doesn't double as MDX_KEY_NO.
  bool handle_toggle_button(gui_event_t *event);

  // FUNC + ARROW: dispatches the corresponding MD window via the 0x40
  // GUI commands (mute/swing/slide/accent). Runs regardless of latch
  // state — FUNC chord beats SPS-mode arrow→bank.
  bool handle_func_arrow_chord(gui_event_t *event);

  // BUTTON1 in SPS-mode → bank-group toggle (A-D ↔ E-H). Flips
  // MD.currentBank locally and bounces MD_GUI_BANKGROUP to SPS so its
  // bank-group LED follows.
  bool handle_button1(gui_event_t *event);

  // Latched arrow → MDX_KEY_BANKA..D, with key-repeat suppression so
  // the bank popup doesn't reopen on every TBD auto-repeat tick.
  bool handle_arrow_to_bank(gui_event_t *event);

  // Latched trig press/release → MD_GUI_TRIG_* sysex, but only on
  // pages that don't already own the trig pad. Caller has already
  // computed the 0..15 trig index.
  bool handle_trig_forward(gui_event_t *event, uint8_t trig_idx);

  // Hijack the four hardware encoders to send live kit-param updates
  // for the current MD synth page. Call from MCL::loop. No lock
  // storage — this is just live tweaking like the MD's own encoders.
  void poll_encoders();

private:
  bool latched_ = false;

  void set_latched(bool v);
};

extern SpsMode sps_mode;
