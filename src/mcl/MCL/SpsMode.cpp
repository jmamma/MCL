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
#include "BankPopupPage.h"

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
          pg == PAGE_SELECT_PAGE || pg == BANK_POPUP_PAGE);
}

void SpsMode::set_latched(bool v) {
  latched_ = v;
  GUI_hardware.led.sps_active = v;
  GUI_hardware.led.updateLeds = true;
  if (v) {
    resync_from_kit();
  } else if (mcl.currentPage() == BANK_POPUP_PAGE) {
    bank_popup_page.close();
  }
}

void SpsMode::resync_from_kit() {
  bound_track_ = MD.currentTrack;
  bound_sub_page_ = sub_page_;
  uint8_t base = param_base();
  for (uint8_t i = 0; i < 4; i++) {
    uint8_t param = base + i;
    if (param >= MD_PARAMS_PER_TRACK) {
      enc[i].setValue(0);
      continue;
    }
    enc[i].setValue(MD.kit.params[MD.currentTrack][param]);
  }
}

void SpsMode::send_param(uint8_t i) {
  if (!MD.connected) return;
  uint8_t param = param_base() + i;
  if (param >= MD_PARAMS_PER_TRACK) return;
  uint8_t v = (uint8_t)enc[i].cur;
  MD.setTrackParam(MD.currentTrack, param, v, nullptr, true);
}

bool SpsMode::handle_toggle_button(gui_event_t *event) {
  if (event->source != ButtonsClass::TBD_KEY_SPS_TOGGLE) return false;
  // Toggle the latch on release rather than press so TR can be held as
  // a modifier (TR + arrow → sub-page traversal, TL → TR → SYSTEM_PAGE).
  // tr_consumed_ is set by chord/arrow handlers during the hold; if any
  // gesture used the TR hold, the release does NOT flip the latch.
  if (is_press(event)) {
    tr_consumed_ = false;
  } else if (is_release(event)) {
    if (!tr_consumed_) set_latched(!latched_);
    tr_consumed_ = false;
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

bool SpsMode::handle_cluster_menus(gui_event_t *event) {
  if (!latched_) return false;
  // BUTTON4=X → MD YES; BUTTON3=Y → MD SCALE (with FUNC variant for the
  // scale-window shortcut). BUTTON1=A is handled globally in
  // tbd_handleEvent (always-on MDX_KEY_NO behaviour, not just SPS).
  // Press AND release are consumed so the local YES/load and shift
  // handlers in MCL pages don't run on either edge while latched.
  if (event->source != ButtonsClass::BUTTON3 &&
      event->source != ButtonsClass::BUTTON4) return false;
  if (!MD.connected) return true;

  const bool func_held = key_interface.is_key_down(MDX_KEY_FUNC);
  // SCALE is the only key with both basic-key press/release and a
  // separate FUNC-shortcut (toggle_scale_window). Track whether we sent
  // hold_scale_button on the press edge so release pairs cleanly.
  static bool scale_held = false;

  if (is_press(event)) {
    switch (event->source) {
      case ButtonsClass::BUTTON4:                                // X → YES
        // Behave as a real MDX_KEY_YES: transmit to MD AND fire the
        // local key_event so cmd_key_state + EVENT_CMD propagation
        // (MCL pages keying off MDX_KEY_YES) match the AVR YES button.
        MD.press_yes_button();
        key_interface.key_event(MDX_KEY_YES, false);
        break;
      case ButtonsClass::BUTTON3:                                // Y → SCALE
        if (func_held) {
          MD.toggle_scale_window();
        } else {
          MD.hold_scale_button();
          scale_held = true;
        }
        break;
      default: break;
    }
  } else if (is_release(event)) {
    switch (event->source) {
      case ButtonsClass::BUTTON4:
        MD.release_yes_button();
        key_interface.key_event(MDX_KEY_YES, true);
        break;
      case ButtonsClass::BUTTON3:
        if (scale_held) {
          MD.release_scale_button();
          scale_held = false;
        }
        break;
      default: break;
    }
  }
  return true;
}

bool SpsMode::handle_arrow_subpage(gui_event_t *event) {
  // Either MCL_B (TBD_KEY_SPS) or TR (TBD_KEY_SPS_TOGGLE) held + arrow
  // cycles sub_page_. Works with the latch off too — the gesture is
  // "modifier held", not "SPS-mode active". When TR is the modifier we
  // mark it consumed so the eventual TR release doesn't flip the latch.
  if (!is_arrow_source(event->source)) return false;
  const bool b_held  = BUTTON_DOWN(ButtonsClass::TBD_KEY_SPS);
  const bool tr_held = BUTTON_DOWN(ButtonsClass::TBD_KEY_SPS_TOGGLE);
  if (!b_held && !tr_held) return false;
  if (is_press(event)) {
    if (tr_held) tr_consumed_ = true;
    int8_t delta = 0;
    switch (event->source) {
      case ButtonsClass::FUNC_BUTTON6: // UP
      case ButtonsClass::FUNC_BUTTON7: // LEFT
        delta = -1; break;
      case ButtonsClass::FUNC_BUTTON8: // DOWN
      case ButtonsClass::FUNC_BUTTON9: // RIGHT
        delta = +1; break;
      default: break;
    }
    sub_page_ = (uint8_t)((sub_page_ + 8 + delta) % 8);
    resync_from_kit();
  }
  return true;
}

bool SpsMode::handle_sps_key_tap(gui_event_t *event) {
  if (event->source != ButtonsClass::TBD_KEY_SPS) return false;
  // FUNC + B: one-shot LFO menu toggle. Bare B: single press_page_button
  // on the press edge — no release counterpart. The MD's PAGE handler
  // (MDX_KEY_HANDLER_0x39) lacks a press-edge gate, so it runs on both
  // edges; sending only one edge gives a single page advance per tap.
  if (is_press(event)) {
    if (latched_ && MD.connected) {
      const bool func_held = key_interface.is_key_down(MDX_KEY_FUNC);
      if (func_held) {
        MD.toggle_lfo_menu();
      } else {
        MD.press_page_button();
      }
    }
  }
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
  if (mcl.currentPage() == BANK_POPUP_PAGE) return false;

  if (is_press(event)) {
    MD.hold_trig(trig_idx + 1);
  } else if (is_release(event)) {
    MD.release_trig(trig_idx + 1);
  }
  return true;
}

bool SpsMode::show_value(uint8_t i) const {
  // Visible only for SHOW_VALUE_TIMEOUT after the last rotation. Press
  // alone does NOT pop the value — pressing ENC1 is the PageSelect tap
  // gesture, and we don't want the strip to flash the value during it.
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
    encs[i] = (param < MD_PARAMS_PER_TRACK) ? &enc[i] : nullptr;
    labels[i] = (param < MD_PARAMS_PER_TRACK)
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
bool SpsMode::handle_cluster_menus(gui_event_t *) { return false; }
bool SpsMode::handle_arrow_subpage(gui_event_t *) { return false; }
bool SpsMode::handle_sps_key_tap(gui_event_t *) { return false; }
bool SpsMode::handle_trig_forward(gui_event_t *, uint8_t) { return false; }
void SpsMode::poll_encoders() {}
void SpsMode::draw_strip(uint8_t) {}
void SpsMode::resync_from_kit() {}
void SpsMode::send_param(uint8_t) {}
bool SpsMode::encoder_passthrough_page() const { return true; }
void SpsMode::set_latched(bool) {}

#endif // PLATFORM_TBD
