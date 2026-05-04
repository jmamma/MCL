/* TBD-only. SPS-mode param-page-select overlay as a real LightPage.
 *
 * Pushed by SpsMode::poll_page_overlay when TR (TBD_KEY_SPS_TOGGLE) has
 * been held past TBD_OVERLAY_HOLD_MS while latched. Sticky — stays on
 * the page stack until popped by:
 *   - TR tap inside the overlay (tap-close gesture)
 *   - any in-page close (B tap, etc.) wired in handleEvent
 *
 * Owns nothing the rest of the system polls per-frame: lifecycle is
 * explicit via init() / cleanup(). LED palette + status LED2 white are
 * established in init() and torn down in cleanup() so we can never
 * leak state across pages.
 *
 * Uses sps_mode.enc[4] for live encoder values (poll_encoders in
 * MCL::loop continues to drive them while latched, so the page just
 * reads sps_mode state).
 */

#pragma once

#ifdef PLATFORM_TBD

#include "GUI.h"
#include "PageIndex.h"

class SpsOverlayPage : public LightPage {
public:
  SpsOverlayPage() : LightPage() {}

  // Pop the overlay back to the previous page.
  void close();

  virtual void setup() override;
  virtual void init() override;
  virtual void cleanup() override;
  virtual void loop() override;
  virtual void display() override;
  virtual bool handleEvent(gui_event_t *event) override;

private:
  // Last sub_page we painted; 255 = nothing painted yet (force repaint
  // on init / re-entry).
  uint8_t painted_sub_page_ = 255;
  // PageIndex to return to when closing. NULL_PAGE = caller didn't set.
  PageIndex prev_page_ = NULL_PAGE;

  // Repaint the trig-LED palette: each available column-pair lights
  // red, the focused sub-page lights white. Idempotent.
  void paint_leds();
};

extern SpsOverlayPage sps_overlay_page;

#endif // PLATFORM_TBD
