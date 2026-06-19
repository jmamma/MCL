/* TBD-only. SPS-mode bottom-32 4-encoder strip as a render overlay.
 *
 * Available as the compact SPS overlay variant. SPS defaults to the
 * full-screen SpsOverlayPage when the device UI is selected, but the
 * panel display-mode chord can toggle into this strip.
 *
 * Renders only y=32..63, so the active page's top-half display
 * remains visible underneath. Reads sps_mode.enc[] for live values
 * and falls back to lock values via sps_mode.active_step_lock when
 * a trig is held in step edit.
 *
 * This is the overlay-based equivalent of the older
 * SpsMode::draw_strip(uint8_t) call that pages used to invoke from
 * inside their own display() — pulling it out lets per-driver "modes"
 * own their own strip overlays without each page knowing about them.
 */

#pragma once

#ifdef PLATFORM_TBD

#include "GUI.h"

class SpsStripPage : public LightPage {
public:
  SpsStripPage() : LightPage() {}

  virtual void setup() override {}
  virtual void init() override;
  virtual void cleanup() override;
  virtual void loop() override;
  virtual void display() override;
  // No event handler — events flow to the active page underneath.

private:
  uint8_t painted_sub_page_ = 255;

  void paint_leds();
};

extern SpsStripPage sps_strip_page;

#endif // PLATFORM_TBD
