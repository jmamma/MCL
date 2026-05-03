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
  bound_page_  = MD.currentSynthPage;
  uint8_t base = MD.currentSynthPage * 8;
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
  uint8_t base = MD.currentSynthPage * 8;
  uint8_t param = base + i;
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
  if (!latched_ || !MD.connected) return false;
  if (trig_idx >= 16) return false;
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

  // Track / synth-page change: bring our cur values back in sync with
  // the kit so the encoder strip doesn't show a stale snapshot.
  if (bound_track_ != MD.currentTrack || bound_page_ != MD.currentSynthPage) {
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

void SpsMode::draw_strip(uint8_t y_top) {
  if (!latched_) return;
  if (encoder_passthrough_page()) return;

  Encoder *encs[4];
  const char *labels[4];
  bool show[4];
  uint8_t base = MD.currentSynthPage * 8;
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
void SpsMode::draw_strip(uint8_t) {}
void SpsMode::resync_from_kit() {}
void SpsMode::send_param(uint8_t) {}
bool SpsMode::encoder_passthrough_page() const { return true; }
void SpsMode::set_latched(bool) {}

#endif // PLATFORM_TBD
