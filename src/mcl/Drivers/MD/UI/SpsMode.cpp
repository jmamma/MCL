#include "SpsMode.h"

#ifdef PLATFORM_TBD
#include "MCL.h"
#include "../MD.h"
#include "../MDParams.h"
#include "GUI/Pages/Grid/GridPage.h"
#include "GUI/Pages/Grid/GridPages.h"
#include "KeyInterface.h"
#include "GUI_hardware.h"
#include "MCLGUI.h"
#include "ResourceManager.h"
#include "GUI/Pages/BankPopupPage.h"
#include "SpsOverlayPage.h"
#include "SpsStripPage.h"
#include "GUI/Pages/Sequencer/SeqPages.h"
#include "Sequencer/MCLSeq.h"
#include "NoteInterface.h"

namespace {

inline bool is_arrow_source(uint8_t source) {
  return source == ButtonsClass::FUNC_BUTTON6 ||
         source == ButtonsClass::FUNC_BUTTON7 ||
         source == ButtonsClass::FUNC_BUTTON8 ||
         source == ButtonsClass::FUNC_BUTTON9;
}

inline bool is_sps_key_source(uint8_t source) {
  return source == ButtonsClass::FUNC_BUTTON5;
}

inline bool is_press(const gui_event_t *event) {
  return event->mask == EVENT_BUTTON_PRESSED;
}

inline bool is_release(const gui_event_t *event) {
  return !(event->mask & 1);
}

// XOR-invert a rectangle in the OLED framebuffer. Adafruit GFX's
// drawPixel(x,y,c) treats any color != WHITE && != BLACK as XOR, so
// passing 2 toggles each pixel. Used to mark locked encoder slots.
static void invert_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
  for (uint8_t yy = y; yy < y + h; yy++) {
    for (uint8_t xx = x; xx < x + w; xx++) {
      oled_display.drawPixel(xx, yy, 2);
    }
  }
}

} // namespace

bool SpsMode::encoder_passthrough_page() const {
  // While SPS is latched the kit-param encoders take precedence on
  // most pages, including the sequencer step / ptc / extstep pages
  // (the user expects to keep tweaking kit params while editing
  // patterns). Only pages that genuinely depend on the encoder
  // column for their core gesture (page select, bank popup) opt out.
  PageIndex pg = mcl.currentPage();
  return (pg == PAGE_SELECT_PAGE || pg == BANK_POPUP_PAGE);
}

void SpsMode::set_latched(bool v) {
  latched_ = v;
  GUI_hardware.led.sps_active = v;
  GUI_hardware.led.updateLeds = true;
  if (v) {
    resync_from_kit();
    GUI.setOverlay(&sps_overlay_page);
  } else {
    // Latch off — clear any SPS overlay we installed.
    if (GUI.overlay == &sps_strip_page || GUI.overlay == &sps_overlay_page) {
      GUI.clearOverlay();
    }
    if (mcl.currentPage() == BANK_POPUP_PAGE) {
      bank_popup_page.close();
    }
  }
}

bool SpsMode::is_collapsed() const {
  return latched_ && GUI.overlay == &sps_strip_page;
}

uint8_t SpsMode::param_count() const {
  return MD.is_spsx ? SPS_PARAMS_PER_TRACK : MD_PARAMS_PER_TRACK;
}

uint8_t SpsMode::param_window_count() const {
  return (uint8_t)((param_count() + 3) / 4);
}

bool SpsMode::active_step_lock(uint8_t param, uint8_t *value) const {
  if (mcl.currentPage() != SEQ_STEP_PAGE) return false;
  if (last_primary_track >= NUM_MD_TRACKS) return false;
  if (note_interface.notes_count_on() == 0) return false;

  const uint8_t param_limit =
      mcl_seq.using_spsx_tracks ? SPS_PARAMS_PER_TRACK : MD_PARAMS_PER_TRACK;
  if (param >= param_limit) return false;

  // First held trig wins (mirrors SeqStepPage's first-md-note rule).
  for (uint8_t i = 0; i < 16; i++) {
    if (!note_interface.is_note_on(i)) continue;
    const uint16_t step = i + (SeqPage::page_select * 16);

    uint8_t locks[SPS_PARAMS_PER_TRACK];
    memset(locks, 255, sizeof(locks));
#if !defined(__AVR__)
    if (mcl_seq.using_spsx_tracks) {
      mcl_seq.spsx_tracks[last_primary_track].get_step_locks(step, locks, false);
    } else
#endif
    {
      mcl_seq.md_tracks[last_primary_track].get_step_locks(step, locks, false);
    }
    if (locks[param] != 255) {
      *value = locks[param];
      return true;
    }
    return false;
  }
  return false;
}

void SpsMode::resync_from_kit() {
  bound_track_ = MD.currentTrack;
  bound_sub_page_ = sub_page_;
  // MD.currentTrack should always be 0..15 but a stray track-index
  // reply has been observed as 24 — fall back to 0 rather than OOB.
  const uint8_t track = (MD.currentTrack < 16) ? MD.currentTrack : 0;
  uint8_t base = param_base();
  for (uint8_t i = 0; i < 4; i++) {
    uint8_t param = base + i;
    if (param >= param_count()) {
      enc[i].setValue(0);
      continue;
    }
    enc[i].setValue(MD.kit.params[track][param]);
  }
}

void SpsMode::send_param(uint8_t i) {
  if (!MD.connected) return;
  uint8_t param = param_base() + i;
  if (param >= param_count()) return;
  uint8_t v = (uint8_t)enc[i].cur;

  // Step-edit + trig held: record a parameter lock on each held step
  // for the active MD track, mirroring SeqStepMidiEvents::onControl-
  // ChangeCallback_Midi (the inbound-CC path). Don't update the kit /
  // send a CC out — locks override the kit per-step at playback.
  const PageIndex pg = mcl.currentPage();
  const bool on_step_page = (pg == SEQ_STEP_PAGE);
  if (on_step_page && note_interface.notes_count_on() > 0 &&
      last_primary_track < NUM_MD_TRACKS) {
    const uint8_t param_limit = mcl_seq.using_spsx_tracks
                                    ? SPS_PARAMS_PER_TRACK
                                    : MD_PARAMS_PER_TRACK;
    if (param >= param_limit) return;
#if !defined(__AVR__)
    if (mcl_seq.using_spsx_tracks) {
      SPSXSeqTrack &st = mcl_seq.spsx_tracks[last_primary_track];
      uint16_t first_step = 0xFFFF;
      for (uint8_t n = 0; n < 16; n++) {
        if (!note_interface.is_note_on(n)) continue;
        const uint16_t step = n + (SeqPage::page_select * 16);
        if (step >= st.length) continue;
        st.set_track_locks(step, param, v);
        if (first_step == 0xFFFF) first_step = step;
      }
      if (first_step != 0xFFFF) {
        uint8_t params[SPS_PARAMS_PER_TRACK];
        memset(params, 255, sizeof(params));
        st.get_step_locks(first_step, params, true);
        MD.activate_encoder_interface(params, SPS_PARAMS_PER_TRACK);
      }
      return;
    }
#endif
    MDSeqTrack &active = mcl_seq.md_tracks[last_primary_track];
    uint16_t first_step = 0xFFFF;
    for (uint8_t n = 0; n < 16; n++) {
      if (!note_interface.is_note_on(n)) continue;
      const uint16_t step = n + (SeqPage::page_select * 16);
      if (step >= active.length) continue;
      active.set_track_locks(step, param, v);
      if (first_step == 0xFFFF) first_step = step;
    }
    // Notify MD which params now carry a lock at this step (mirrors
    // SeqStepPage::send_locks → MD.activate_encoder_interface).
    if (first_step != 0xFFFF) {
      uint8_t params[SPS_PARAMS_PER_TRACK];
      memset(params, 255, sizeof(params));
      active.get_step_locks(first_step, params, true);
      MD.activate_encoder_interface(params, MD_PARAMS_PER_TRACK);
    }
    return;
  }

  // Default: send CC + update kit (drives the live MD synth voice).
  MD.setTrackParam(MD.currentTrack, param, v, nullptr, true);
}

bool SpsMode::handle_func_arrow_chord(gui_event_t *event) {
  if (!latched_) return false;
  if (!is_arrow_source(event->source)) return false;
  if (!key_interface.is_key_down(MDX_KEY_FUNC)) return false;
  if (MD.connected && is_press(event)) {
    switch (event->source) {
      case ButtonsClass::FUNC_BUTTON6:
        mcl_remote_func_window_replaced();
        MD.toggle_accent_window();
        break; // UP
      case ButtonsClass::FUNC_BUTTON9:
        mcl_remote_func_window_replaced();
        MD.toggle_swing_window();
        break; // RIGHT
      case ButtonsClass::FUNC_BUTTON8:
        mcl_remote_func_window_replaced();
        MD.toggle_slide_window();
        break; // DOWN
      case ButtonsClass::FUNC_BUTTON7:
        mcl_toggle_remote_mute_window();
        break; // LEFT
      default: break;
    }
  }
  return true;
}

bool SpsMode::handle_cluster_menus(gui_event_t *event) {
  if (!latched_) return false;
  bool yes_key = false;
  bool scale_key = false;
  bool func_key = false;
  uint8_t key = 255;
  switch (event->source) {
    case ButtonsClass::BUTTON4: // X
      yes_key = true;
      key = MDX_KEY_YES;
      break;
    case ButtonsClass::BUTTON1: // A
    case ButtonsClass::BUTTON3: // Y legacy shortcut
      key = MDX_KEY_NO;
      break;
    case ButtonsClass::TBD_BUTTON_B: // B
      scale_key = true;
      key = MDX_KEY_SCALE;
      break;
    case ButtonsClass::FUNC_BUTTON5: // FUNC
      func_key = true;
      key = MDX_KEY_FUNC;
      break;
    default:
      return false;
  }

  if (!MD.connected) return true;

  if (is_press(event)) {
    if (scale_key) {
      if (key_interface.is_key_down(MDX_KEY_FUNC) ||
          BUTTON_DOWN(ButtonsClass::FUNC_BUTTON5)) {
        mcl_remote_func_window_replaced();
        MD.toggle_scale_window();
        scale_key_held_ = false;
      } else {
        MD.hold_scale_button();
        key_interface.key_event(MDX_KEY_SCALE, false);
        scale_key_held_ = true;
      }
    } else if (func_key) {
      sps_key_consumed_ = false;
      MD.hold_function_button();
      key_interface.set_key_state(MDX_KEY_FUNC, true);
    } else if (yes_key) {
      MD.press_yes_button();
    } else {
      MD.press_no_button();
    }
    if (!scale_key && !func_key) {
      key_interface.set_key_state(key, true);
    }
  } else if (is_release(event)) {
    if (scale_key) {
      if (scale_key_held_) {
        MD.release_scale_button();
        key_interface.key_event(MDX_KEY_SCALE, true);
      }
      scale_key_held_ = false;
    } else if (func_key) {
      MD.release_function_button();
      key_interface.set_key_state(MDX_KEY_FUNC, false);
      sps_key_consumed_ = false;
    } else if (yes_key) {
      MD.release_yes_button();
    } else {
      MD.release_no_button();
    }
    if (!scale_key && !func_key) {
      key_interface.set_key_state(key, false);
    }
  }
  return true;
}

bool SpsMode::handle_arrow_subpage(gui_event_t *event) {
  // Either the physical FUNC key or the logical device UI button held
  // + arrow cycles sub_page_.
  if (!is_arrow_source(event->source)) return false;
  const bool sps_key_held = BUTTON_DOWN(ButtonsClass::FUNC_BUTTON5);
  const bool ui_button_held = ui_button_pressed_;
  DEBUG_PRINT("    SpsMode::handle_arrow_subpage latched=");
  DEBUG_PRINT((unsigned)latched_);
  DEBUG_PRINT(" sps_key=");
  DEBUG_PRINT((unsigned)sps_key_held);
  DEBUG_PRINT(" ui_button=");
  DEBUG_PRINTLN((unsigned)ui_button_held);
  if (sps_key_held && mcl.currentPage() == GRID_PAGE && grid_page.show_slot_menu) {
    if (is_press(event)) sps_key_consumed_ = true;
    return false;
  }
  const bool overlay_active = (GUI.overlay == &sps_overlay_page);
  // While the SPS overlay is active (the 8-encoder page select view)
  // or a modifier is held, the cluster owns sub-page traversal.
  // Otherwise, panel arrows fall through to grid / seq navigation by
  // default (even if the SPS latch is on).
  if (!overlay_active && !sps_key_held && !ui_button_held) {
    DEBUG_PRINTLN("    -> reject (no modifier, no overlay)");
    return false;
  }
  if (is_press(event)) {
    // Suppress key-repeat: a held arrow only fires once per physical
    // press. The release branch clears arrow_consumed_source_ so the
    // next press of any arrow is honoured.
    if (arrow_consumed_source_ == event->source) return true;
    arrow_consumed_source_ = event->source;

    if (sps_key_held) sps_key_consumed_ = true;
    // sub_page_ is the 4-param column id (0..7). The 8-param "page" is
    // sub_page_ >> 1, the half within the page is sub_page_ & 1.
    //   UP    → upper half of the current page.
    //   DOWN  → lower half of the current page, if it exists.
    //   LEFT/RIGHT → step pages by ±2 columns, clamped.
    // Stock MD firmware exposes 24 params; SPS firmware exposes 34.
    // max_columns clips the
    // wrap range so we don't scroll into blank pages on a stock MD.
    const uint8_t max_columns = param_window_count();
    if (sub_page_ >= max_columns) sub_page_ = max_columns - 1;

    switch (event->source) {
      case ButtonsClass::FUNC_BUTTON6: // UP
        sub_page_ &= 0xFE;
        break;
      case ButtonsClass::FUNC_BUTTON8: { // DOWN
        const uint8_t next = sub_page_ | 1;
        if (next < max_columns) sub_page_ = next;
        break;
      }
      case ButtonsClass::FUNC_BUTTON7: // LEFT
      case ButtonsClass::FUNC_BUTTON9: { // RIGHT
        const int8_t delta = (event->source == ButtonsClass::FUNC_BUTTON7) ? -2 : +2;
        const int next = (int)sub_page_ + delta;
        // Clamp at the edges — no wrap. Outside arrows become no-ops.
        if (next >= 0 && next < (int)max_columns) {
          sub_page_ = (uint8_t)next;
        }
        break;
      }
      default: break;
    }
    resync_from_kit();
  } else if (is_release(event)) {
    if (arrow_consumed_source_ == event->source) arrow_consumed_source_ = 255;
  }
  return true;
}

void SpsMode::observe_sps_key_chord(gui_event_t *event) {
  if (!EVENT_BUTTON(event)) return;
  if (!is_press(event)) return;
  if (is_sps_key_source(event->source)) return;
  if (BUTTON_DOWN(ButtonsClass::FUNC_BUTTON5)) {
    sps_key_consumed_ = true;
  }
}

bool SpsMode::handle_sps_key_tap(gui_event_t *event) {
  if (!is_sps_key_source(event->source)) return false;
  // The physical FUNC key does NOT toggle the SPS latch — that's TR's
  // role. Tap fires a per-page action; any other button press while it
  // is down suppresses the release tap action.
  //   GRID_PAGE  → flip cur_grid (0/1 trig-grid view)
  //   SEQ_*      → advance sequencer page (BUTTON4 analog on AVR)
  if (is_press(event)) {
    sps_key_consumed_ = false;
  } else if (is_release(event)) {
    if (!sps_key_consumed_) {
      const PageIndex pg = mcl.currentPage();
      if (pg == GRID_PAGE) {
        // GridPage::handleEvent already implements the cur_grid swap
        // on MDX_KEY_SCALE press (resets param1.max, re-init). Post
        // that event so we get the full sequence — not just the bit
        // flip.
        key_interface.post_key_event(MDX_KEY_SCALE, false);
      } else if (pg == SEQ_STEP_PAGE || pg == SEQ_PTC_PAGE ||
                 pg == SEQ_EXTSTEP_PAGE) {
        // Synthesize the BUTTON4 release that SeqPage::handleEvent
        // already handles for page-select advance — avoids duplicating
        // the track-length / page-count clamp that lives there.
        gui_event_t e;
        e.type = BUTTON;
        e.source = ButtonsClass::BUTTON4;
        e.mask = EVENT_BUTTON_RELEASED;
        e.port = 0;
        e.modifiers = 0;
        GUI.putEvent(&e);
      }
    }
    sps_key_consumed_ = false;
  }
  return true;
}

bool SpsMode::handle_trig_forward(gui_event_t *event, uint8_t trig_idx) {
  if (!latched_) return false;
  if (trig_idx >= 16) return false;

  // Trig sub-page selection requires an explicit modifier: physical FUNC
  // or the logical device UI button held. Without a modifier, trigs fall
  // through to the active page — so on SEQ_STEP_PAGE you can keep
  // entering parameter locks (trig press = step select), on GRID_PAGE
  // you keep triggering the track, etc. Latch state alone no longer
  // claims trigs.
  const bool sps_key_held = BUTTON_DOWN(ButtonsClass::FUNC_BUTTON5);
  const bool ui_button_held = ui_button_pressed_;
  if (!sps_key_held && !ui_button_held) return false;

  if (is_press(event)) {
    const uint8_t max_sub_pages = param_window_count();
    const uint8_t page_col = trig_idx & 0x07;
    const uint8_t half = (trig_idx >= 8) ? 1 : 0;
    const uint8_t target = page_col * 2 + half;
    if (target < max_sub_pages) {
      sub_page_ = target;
      resync_from_kit();
    }
    if (sps_key_held) sps_key_consumed_ = true;
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

void SpsMode::handle_ui_slot_button(bool pressed) {
  if (pressed) {
    ui_button_press_ms_ = read_clock_ms();
    ui_button_pressed_ = true;
    ui_button_hold_handled_ = false;
  } else {
    ui_button_pressed_ = false;
    ui_button_hold_handled_ = false;
  }
}

void SpsMode::poll_page_overlay() {
  if (!latched_ || !ui_button_pressed_ || ui_button_hold_handled_) return;
  if (clock_diff(ui_button_press_ms_, read_clock_ms()) <=
      SPS_FULLSCREEN_HOLD_MS) {
    return;
  }
  if (GUI.overlay == &sps_overlay_page) {
    GUI.setOverlay(&sps_strip_page);
  } else {
    GUI.setOverlay(&sps_overlay_page);
  }
  ui_button_hold_handled_ = true;
}

#else // !PLATFORM_TBD

bool SpsMode::handle_func_arrow_chord(gui_event_t *) { return false; }
bool SpsMode::handle_cluster_menus(gui_event_t *) { return false; }
bool SpsMode::handle_arrow_subpage(gui_event_t *) { return false; }
void SpsMode::observe_sps_key_chord(gui_event_t *) {}
bool SpsMode::handle_sps_key_tap(gui_event_t *) { return false; }
bool SpsMode::handle_trig_forward(gui_event_t *, uint8_t) { return false; }
void SpsMode::poll_encoders() {}
void SpsMode::handle_ui_slot_button(bool) {}
void SpsMode::poll_page_overlay() {}
void SpsMode::resync_from_kit() {}
bool SpsMode::active_step_lock(uint8_t, uint8_t *) const { return false; }
void SpsMode::send_param(uint8_t) {}
uint8_t SpsMode::param_count() const { return 0; }
uint8_t SpsMode::param_window_count() const { return 0; }
bool SpsMode::encoder_passthrough_page() const { return true; }
void SpsMode::set_latched(bool) {}
bool SpsMode::is_collapsed() const { return false; }

#endif // PLATFORM_TBD
