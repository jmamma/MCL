/* TBD-only. SPS-mode param-page-select overlay.
 *
 * NOT a real page — installed via GUI.setOverlay() so it renders on
 * top of whatever page is currently active without touching the page
 * stack or currentPage(). The active page (e.g. SeqStepPage) keeps
 * full ownership of events, encoders, and its own display; the
 * overlay just adds a render pass + an LED palette tick.
 *
 * Lifecycle is owned by SpsMode:
 *   - SpsMode::enable defaults to GUI.setOverlay(&sps_overlay_page).
 *   - The panel display-mode chord toggles between this full-screen
 *     overlay and the compact strip.
 *   - SpsMode::disable clears SPS overlays when the device UI is exited.
 *
 * Trig / arrow sub-page navigation while this overlay is active, or
 * while the logical device UI button is held, is handled by SpsMode's
 * existing handlers (handle_arrow_subpage, handle_trig_forward) via
 * tbd_handleEvent.
 */

#pragma once

#ifdef PLATFORM_TBD

#include "GUI.h"

class SpsOverlayPage : public LightPage {
public:
  SpsOverlayPage() : LightPage() {}

  virtual void setup() override {}
  virtual void init() override;
  virtual void cleanup() override;
  virtual void loop() override;
  virtual void display() override;
  // No handleEvent — events flow to the active page underneath.

private:
  // Last sub_page we painted to LEDs; 255 = nothing painted.
  uint8_t painted_sub_page_ = 255;

  // Repaint trig-LED palette: each available column-pair lights red,
  // the focused sub-page lights white. Idempotent.
  void paint_leds();
};

extern SpsOverlayPage sps_overlay_page;

#endif // PLATFORM_TBD
