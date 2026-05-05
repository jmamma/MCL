/* TBD-only. Bank-popup page: owns ENC1 (range 0..7, wraps) for letter
 * cycling, draws the 2x4 bank grid overlay, and forwards trig presses
 * to the same load/chain machinery the AVR uses (state lives on
 * grid_page so the SeqPage / SpsMode / cross-page queries keep working
 * unchanged).
 *
 * Push with mcl.pushPage(BANK_POPUP_PAGE); close with
 * bank_popup_page.close() (or any of the in-page close gestures —
 * ENC1 tap, MCL_A / NO).
 */

#pragma once

#ifdef PLATFORM_TBD

#include "GUI.h"
#include "MCLEncoder.h"

class BankPopupPage : public LightPage {
public:
  BankPopupPage(MCLEncoder *e1) : LightPage(e1) {}

  // Pop the page and tear down the popup state on grid_page.
  void close();

  virtual void setup() override;
  virtual void init() override;
  virtual void cleanup() override;
  virtual void loop() override;
  virtual void display() override;
  virtual bool handleEvent(gui_event_t *event) override;

private:
  // Apply a +1 / -1 letter step (or group flip) and resync MD.currentBank
  // when the new bank crosses a group boundary. Repaints row LEDs.
  void step_bank(int8_t letter_delta, bool group_toggle);
  // Repaint trig LEDs from the popup state (head=red, chained=yellow,
  // populated=dim red baseline). Called on each new trig selection.
  void repaint_chain_leds();

  // Cached previous encoder value so we can derive a delta from the
  // wrapping-aware MCLEncoder. -1 = uninitialised on push.
  int8_t last_enc_ = -1;
};

extern BankPopupPage bank_popup_page;
extern MCLEncoder bank_popup_encoder;

#endif // PLATFORM_TBD
