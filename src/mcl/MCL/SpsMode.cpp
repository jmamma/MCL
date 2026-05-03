#include "SpsMode.h"

#ifdef PLATFORM_TBD
#include "MCL.h"
#include "MD.h"
#include "MDParams.h"
#include "GridPage.h"
#include "GridPages.h"
#include "KeyInterface.h"
#include "GUI_hardware.h"

SpsMode sps_mode;

namespace {

// Panel arrow IDs used throughout. UP=6, LEFT=7, DOWN=8, RIGHT=9 in
// ButtonsClass.
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

void SpsMode::set_latched(bool v) {
  latched_ = v;
  GUI_hardware.led.sps_active = v;
  GUI_hardware.led.updateLeds = true;
  if (!v && grid_page.bank_popup) {
    grid_page.close_bank_popup();
  }
}

bool SpsMode::handle_toggle_button(gui_event_t *event) {
  if (event->source != ButtonsClass::FUNC_BUTTON5) return false;
  if (is_press(event)) {
    set_latched(!latched_);
  }
  return true;
}

bool SpsMode::handle_func_arrow_chord(gui_event_t *event) {
  if (!is_arrow_source(event->source)) return false;
  if (!key_interface.is_key_down(MDX_KEY_FUNC)) return false;
  // Eat both press and release so the arrow's grid navigation modifier
  // and the SPS-mode bank routing don't also fire. MD windows are
  // toggle-on commands so we only fire on press.
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

bool SpsMode::handle_button1(gui_event_t *event) {
  if (!latched_) return false;
  if (event->source != ButtonsClass::BUTTON1) return false;
  if (is_press(event)) {
    MD.currentBank ^= 1;
    if (MD.connected) {
      MD.press_bankgroup_button();
    }
  }
  return true;
}

bool SpsMode::handle_arrow_to_bank(gui_event_t *event) {
  if (!latched_) return false;
  if (!is_arrow_source(event->source)) return false;
  uint8_t key = 255;
  switch (event->source) {
    case ButtonsClass::FUNC_BUTTON7: key = MDX_KEY_BANKA; break; // LEFT
    case ButtonsClass::FUNC_BUTTON8: key = MDX_KEY_BANKB; break; // DOWN
    case ButtonsClass::FUNC_BUTTON9: key = MDX_KEY_BANKC; break; // RIGHT
    case ButtonsClass::FUNC_BUTTON6: key = MDX_KEY_BANKD; break; // UP
    default: return true;
  }
  // The TBD event loop auto-repeats held arrows; ignore repeats so the
  // bank popup doesn't reopen and re-flood MD with LED sysex.
  if (!is_release(event) && key_interface.is_key_down(key)) {
    return true;
  }
  key_interface.key_event(key, is_release(event));
  return true;
}

bool SpsMode::handle_trig_forward(gui_event_t *event, uint8_t trig_idx) {
  if (!latched_ || !MD.connected) return false;
  if (trig_idx >= 16) return false;
  // Pages that own the trig pad keep their local role.
  PageIndex pg = mcl.currentPage();
  if (pg == SEQ_STEP_PAGE || pg == SEQ_PTC_PAGE ||
      pg == SEQ_EXTSTEP_PAGE || pg == PERF_PAGE_0) {
    return false;
  }
  // Bank popup's pattern-load handler still wins on grid page.
  if (grid_page.bank_popup) return false;

  if (is_press(event)) {
    MD.hold_trig(trig_idx + 1);     // hold_trig API is 1-indexed
  } else if (is_release(event)) {
    MD.release_trig(trig_idx + 1);
  }
  return true;
}

void SpsMode::poll_encoders() {
  if (!latched_ || !MD.connected) return;
  PageIndex pg = mcl.currentPage();
  // Pages that own the four encoders for their own purposes — leave them
  // alone. PAGE_SELECT_PAGE in particular needs the encoder to scroll the
  // page list. Step / perf use them for live track/step editing.
  if (pg == SEQ_STEP_PAGE || pg == SEQ_PTC_PAGE ||
      pg == SEQ_EXTSTEP_PAGE || pg == PERF_PAGE_0 ||
      pg == PAGE_SELECT_PAGE) {
    return;
  }
  // Rate-limit to match Encoder::update_rotations (rot_res=1,
  // ENCODER_RES_MULTIPLIER=1) so SPS-mode tweaking feels the same as
  // turning a page-bound encoder. Without this the raw panel delta gives
  // ~2x the apparent speed.
  static int8_t accum[4] = {0, 0, 0, 0};
  const uint8_t base = (uint8_t)(MD.currentSynthPage * 8);
  for (uint8_t i = 0; i < 4; i++) {
    int8_t delta = Encoders.encoders[i].normal;
    if (delta == 0) continue;
    uint8_t param = base + i;
    if (param >= MD_PARAMS_LEGACY) {
      Encoders.encoders[i].normal = 0;
      accum[i] = 0;
      continue;
    }
    accum[i] += delta;
    int inc = 0;
    while (accum[i] >= 2)  { accum[i] -= 2; inc += 1; }
    while (accum[i] <= -2) { accum[i] += 2; inc -= 1; }
    // Eat the delta so the active MCL page's encoders don't also rotate.
    Encoders.encoders[i].normal = 0;
    if (inc == 0) continue;
    int v = (int)MD.kit.params[MD.currentTrack][param] + inc;
    if (v < 0) v = 0;
    if (v > 127) v = 127;
    MD.setTrackParam(MD.currentTrack, param, (uint8_t)v, nullptr, true);
  }
}

#else // !PLATFORM_TBD

SpsMode sps_mode;

bool SpsMode::handle_toggle_button(gui_event_t *) { return false; }
bool SpsMode::handle_func_arrow_chord(gui_event_t *) { return false; }
bool SpsMode::handle_button1(gui_event_t *) { return false; }
bool SpsMode::handle_arrow_to_bank(gui_event_t *) { return false; }
bool SpsMode::handle_trig_forward(gui_event_t *, uint8_t) { return false; }
void SpsMode::poll_encoders() {}
void SpsMode::set_latched(bool) {}

#endif // PLATFORM_TBD
