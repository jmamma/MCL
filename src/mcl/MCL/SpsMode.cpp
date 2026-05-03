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
  PageIndex pg = mcl.currentPage();
  return (pg == SEQ_STEP_PAGE || pg == SEQ_PTC_PAGE ||
          pg == SEQ_EXTSTEP_PAGE || pg == PERF_PAGE_0 ||
          pg == PAGE_SELECT_PAGE);
}

void SpsMode::set_latched(bool v) {
  latched_ = v;
  GUI_hardware.led.sps_active = v;
  GUI_hardware.led.updateLeds = true;
  if (v) {
    resync_from_kit();
  } else if (grid_page.bank_popup) {
    grid_page.close_bank_popup();
  }
}

void SpsMode::resync_from_kit() {
  bound_track_ = MD.currentTrack;
  bound_sub_page_ = sub_page_;
  uint8_t base = param_base();
  for (uint8_t i = 0; i < 4; i++) {
    uint8_t param = base + i;
    if (param >= MD_PARAMS_LEGACY) {
      enc[i].setValue(0);
      continue;
    }
    enc[i].setValue(MD.kit.params[MD.currentTrack][param]);
  }
}

void SpsMode::send_param(uint8_t i) {
  if (!MD.connected) return;
  uint8_t param = param_base() + i;
  if (param >= MD_PARAMS_LEGACY) return;
  uint8_t v = (uint8_t)enc[i].cur;
  MD.setTrackParam(MD.currentTrack, param, v, nullptr, true);
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
  if (!is_release(event) && key_interface.is_key_down(key)) {
    return true; // already down — ignore TBD auto-repeat
  }
  key_interface.key_event(key, is_release(event));
  return true;
}

bool SpsMode::handle_trig_forward(gui_event_t *event, uint8_t trig_idx) {
  if (!latched_) return false;
  if (trig_idx >= 16) return false;

  // MCL_B (TBD_KEY_SPS) held + trig press selects the 4-param window.
  // Each panel column (top trig N + bottom trig N+8) maps to sub_page = N.
  // Eaten so the trig doesn't also forward to the MD as a panel key.
  // We check the button state directly (rather than MDX_KEY_FUNC) because
  // MCL_B and MCL_Y share that key code — only MCL_B is the SPS-mode
  // modifier here.
  if (BUTTON_DOWN(ButtonsClass::TBD_KEY_SPS)) {
    if (is_press(event)) {
      sub_page_ = trig_idx & 0x07;  // collapse 16 trigs into 8 columns
      resync_from_kit();
    }
    return true;
  }

  if (!MD.connected) return false;
  PageIndex pg = mcl.currentPage();
  if (pg == SEQ_STEP_PAGE || pg == SEQ_PTC_PAGE ||
      pg == SEQ_EXTSTEP_PAGE || pg == PERF_PAGE_0) {
    return false;
  }
  if (grid_page.bank_popup) return false;

  if (is_press(event)) {
    MD.hold_trig(trig_idx + 1);
  } else if (is_release(event)) {
    MD.release_trig(trig_idx + 1);
  }
  return true;
}

bool SpsMode::show_value(uint8_t i) const {
  // Mirrors MCLGUI::show_encoder_value: visible while the encoder is
  // pressed, or for SHOW_VALUE_TIMEOUT after the last cur change.
  if (BUTTON_DOWN(Buttons.ENCODER1 + i)) return true;
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
  const bool want_overlay =
      latched_ && BUTTON_DOWN(ButtonsClass::TBD_KEY_SPS);
  if (want_overlay == page_overlay_painted_) {
    // If still painted, refresh in case sub_page_ moved (so the blink
    // tracks the newly-picked column without waiting for a next frame).
    if (want_overlay) {
      // Fall through to repaint.
    } else {
      return;
    }
  }
  if (!want_overlay) {
    // Drop colour-override mode. Page rendering takes back over —
    // anything drawing trig LEDs continuously will repopulate.
    mcl_gui.set_trigleds_local(0, TRIGLED_OVERLAY);
    page_overlay_painted_ = false;
    return;
  }
  // 8 warm hues, one per column. Each column = the top trig (col)
  // paired with the bottom trig (col + 8). Picked to be visually
  // distinct under bright lights but stay in the warm half of the
  // wheel so it reads as "this is the page selector, not a bank".
  static constexpr uint32_t kCols[8] = {
      ((uint32_t)255 << 16) | ((uint32_t)0   << 8) | 0,    // red
      ((uint32_t)255 << 16) | ((uint32_t)48  << 8) | 0,    // red-orange
      ((uint32_t)255 << 16) | ((uint32_t)96  << 8) | 0,    // orange
      ((uint32_t)220 << 16) | ((uint32_t)128 << 8) | 0,    // dark amber
      ((uint32_t)255 << 16) | ((uint32_t)160 << 8) | 0,    // amber
      ((uint32_t)200 << 16) | ((uint32_t)128 << 8) | 24,   // gold
      ((uint32_t)255 << 16) | ((uint32_t)200 << 8) | 0,    // yellow
      ((uint32_t)255 << 16) | ((uint32_t)96  << 8) | 32,   // peach
  };
  mcl_gui.set_trigleds_color(0xFFFF, 0); // clear all
  for (uint8_t c = 0; c < 8; c++) {
    uint16_t mask = ((uint16_t)1 << c) | ((uint16_t)1 << (c + 8));
    mcl_gui.set_trigleds_color(mask, kCols[c]);
  }
  // Highlight the current sub-page column with a blink so the user
  // can tell which 4-param window is in play.
  uint8_t cur = sub_page_ & 0x07;
  uint16_t cur_mask = ((uint16_t)1 << cur) | ((uint16_t)1 << (cur + 8));
  mcl_gui.set_trigleds_blink_color(cur_mask, kCols[cur]);
  page_overlay_painted_ = true;
}

void SpsMode::draw_strip(uint8_t y_top) {
  if (!latched_) return;
  if (encoder_passthrough_page()) return;

  Encoder *encs[4];
  const char *labels[4];
  bool show[4];
  uint8_t base = param_base();
  uint8_t model = MD.kit.get_model(MD.currentTrack);
  for (uint8_t i = 0; i < 4; i++) {
    uint8_t param = base + i;
    encs[i] = (param < MD_PARAMS_LEGACY) ? &enc[i] : nullptr;
    labels[i] = (param < MD_PARAMS_LEGACY)
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
bool SpsMode::handle_button1(gui_event_t *) { return false; }
bool SpsMode::handle_arrow_to_bank(gui_event_t *) { return false; }
bool SpsMode::handle_trig_forward(gui_event_t *, uint8_t) { return false; }
void SpsMode::poll_encoders() {}
void SpsMode::poll_page_overlay() {}
void SpsMode::draw_strip(uint8_t) {}
void SpsMode::resync_from_kit() {}
void SpsMode::send_param(uint8_t) {}
bool SpsMode::encoder_passthrough_page() const { return true; }
void SpsMode::set_latched(bool) {}

#endif // PLATFORM_TBD
