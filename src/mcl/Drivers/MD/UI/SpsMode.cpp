#include "SpsMode.h"

#ifdef PLATFORM_TBD
#include "MCL.h"
#include "../MD.h"
#include "../MDParams.h"
#include "GridPage.h"
#include "GridPages.h"
#include "KeyInterface.h"
#include "GUI_hardware.h"
#include "MCLGUI.h"
#include "ResourceManager.h"
#include "BankPopupPage.h"
#include "SpsOverlayPage.h"
#include "SpsStripPage.h"
#include "SeqPages.h"
#include "MCLSeq.h"
#include "NoteInterface.h"

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
    // Install the bottom-strip overlay so SPS params are visible on
    // top of the active page. Replaced by SpsOverlayPage when TR is
    // held past the overlay threshold.
    if (GUI.overlay != &sps_overlay_page) {
      GUI.setOverlay(&sps_strip_page);
    }
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

bool SpsMode::active_step_lock(uint8_t param, uint8_t *value) const {
  if (mcl.currentPage() != SEQ_STEP_PAGE) return false;
  if (last_md_track >= NUM_MD_TRACKS) return false;
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
      mcl_seq.spsx_tracks[last_md_track].get_step_locks(step, locks, false);
    } else
#endif
    {
      mcl_seq.md_tracks[last_md_track].get_step_locks(step, locks, false);
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
    if (param >= MD_PARAMS_PER_TRACK) {
      enc[i].setValue(0);
      continue;
    }
    enc[i].setValue(MD.kit.params[track][param]);
  }
}

void SpsMode::send_param(uint8_t i) {
  if (!MD.connected) return;
  uint8_t param = param_base() + i;
  if (param >= MD_PARAMS_PER_TRACK) return;
  uint8_t v = (uint8_t)enc[i].cur;

  // Step-edit + trig held: record a parameter lock on each held step
  // for the active MD track, mirroring SeqStepMidiEvents::onControl-
  // ChangeCallback_Midi (the inbound-CC path). Don't update the kit /
  // send a CC out — locks override the kit per-step at playback.
  const PageIndex pg = mcl.currentPage();
  const bool on_step_page = (pg == SEQ_STEP_PAGE);
  if (on_step_page && note_interface.notes_count_on() > 0 &&
      last_md_track < NUM_MD_TRACKS) {
    const uint8_t param_limit = mcl_seq.using_spsx_tracks
                                    ? SPS_PARAMS_PER_TRACK
                                    : MD_PARAMS_PER_TRACK;
    if (param >= param_limit) return;
#if !defined(__AVR__)
    if (mcl_seq.using_spsx_tracks) {
      SPSXSeqTrack &st = mcl_seq.spsx_tracks[last_md_track];
      uint16_t first_step = 0xFFFF;
      for (uint8_t n = 0; n < 16; n++) {
        if (!note_interface.is_note_on(n)) continue;
        const uint16_t step = n + (SeqPage::page_select * 16);
        if (step >= st.length) continue;
        st.set_track_locks(step, param, v);
        st.enable_step_locks(step);
        if (first_step == 0xFFFF) first_step = step;
      }
      if (first_step != 0xFFFF) {
        uint8_t params[SPS_PARAMS_PER_TRACK];
        memset(params, 255, sizeof(params));
        st.get_step_locks(first_step, params, true);
        MD.activate_encoder_interface(params);
      }
      return;
    }
#endif
    MDSeqTrack &active = mcl_seq.md_tracks[last_md_track];
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
      MD.activate_encoder_interface(params);
    }
    return;
  }

  // Default: send CC + update kit (drives the live MD synth voice).
  MD.setTrackParam(MD.currentTrack, param, v, nullptr, true);
}

bool SpsMode::handle_toggle_button(gui_event_t *event) {
  if (event->source != ButtonsClass::TBD_BUTTON_TR) return false;
  // TR semantics:
  //   press                   → toggle SPS latch immediately, OR
  //                             close the overlay if one is up
  //                             (latch unchanged).
  //   hold ≥ TBD_OVERLAY_HOLD_MS → poll_page_overlay installs the
  //                             SPS overlay (sticky).
  if (is_press(event)) {
    tr_press_ms_ = read_clock_ms();
    tr_pressed_ = true;

    if (GUI.overlay == &sps_overlay_page) {
      // Param-page-select overlay open + press → drop back to the
      // bottom strip. Latch unchanged.
      GUI.setOverlay(&sps_strip_page);
    } else {
      set_latched(!latched_);
    }
  } else if (is_release(event)) {
    tr_pressed_ = false;
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
  // BUTTON1=Y → MD SCALE (with FUNC variant for the scale-window
  // shortcut). BUTTON3=A is the global MDX_KEY_NO passthrough
  // handled in tbd_handleEvent; BUTTON4=X is the global
  // MDX_KEY_YES passthrough.
  if (event->source != ButtonsClass::BUTTON1) return false;
  if (!MD.connected) return true;

  const bool func_held = key_interface.is_key_down(MDX_KEY_FUNC);
  // SCALE is the only key with both basic-key press/release and a
  // separate FUNC-shortcut (toggle_scale_window). Track whether we sent
  // hold_scale_button on the press edge so release pairs cleanly.
  static bool scale_held = false;

  if (is_press(event)) {
    if (func_held) {
      MD.toggle_scale_window();
    } else {
      MD.hold_scale_button();
      scale_held = true;
    }
  } else if (is_release(event)) {
    if (scale_held) {
      MD.release_scale_button();
      scale_held = false;
    }
  }
  return true;
}

bool SpsMode::handle_arrow_subpage(gui_event_t *event) {
  // Either B (TBD_BUTTON_B) or TR (TBD_BUTTON_TR) held + arrow
  // cycles sub_page_. Works with the latch off too — the gesture is
  // "modifier held", not "SPS-mode active".
  if (!is_arrow_source(event->source)) return false;
  const bool b_held  = BUTTON_DOWN(ButtonsClass::TBD_BUTTON_B);
  const bool tr_held = BUTTON_DOWN(ButtonsClass::TBD_BUTTON_TR);
  const bool overlay_active = (GUI.overlay == &sps_overlay_page);
  // While the SPS overlay is active (the 8-encoder page select view)
  // or a modifier (B/TR) is held, the cluster owns sub-page traversal.
  // Otherwise, panel arrows fall through to grid / seq navigation by
  // default (even if the SPS latch is on).
  if (!overlay_active && !b_held && !tr_held) return false;
  if (is_press(event)) {
    // Suppress key-repeat: a held arrow only fires once per physical
    // press. The release branch clears arrow_consumed_source_ so the
    // next press of any arrow is honoured.
    if (arrow_consumed_source_ == event->source) return true;
    arrow_consumed_source_ = event->source;

    if (b_held)  b_consumed_  = true;
    // sub_page_ is the 4-param column id (0..7). The 8-param "page" is
    // sub_page_ >> 1, the half within the page is sub_page_ & 1.
    //   UP/DOWN  → flip half (toggle bit 0).
    //   LEFT/RIGHT → cycle page by ±2 columns, with wrap.
    // Stock MD firmware exposes 24 params (3 pages = 6 columns); SPS
    // firmware exposes 32 (4 pages = 8 columns). max_columns clips the
    // wrap range so we don't scroll into blank pages on a stock MD.
    const uint8_t max_columns = MD.is_spsx ? 8 : 6;
    if (sub_page_ >= max_columns) sub_page_ = max_columns - 1;

    switch (event->source) {
      case ButtonsClass::FUNC_BUTTON6: // UP
      case ButtonsClass::FUNC_BUTTON8: // DOWN
        sub_page_ ^= 1;
        if (sub_page_ >= max_columns) sub_page_ = max_columns - 1;
        break;
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

bool SpsMode::handle_sps_key_tap(gui_event_t *event) {
  if (event->source != ButtonsClass::TBD_BUTTON_B) return false;
  // B does NOT toggle the SPS latch — that's TR's role. Tap fires a
  // per-page action; hold (with arrow / trig chord) is the SPS sub-page
  // modifier (handle_arrow_subpage, handle_trig_forward). b_consumed_
  // suppresses the tap action when a chord ran during the hold.
  //   GRID_PAGE  → flip cur_grid (0/1 trig-grid view)
  //   SEQ_*      → advance sequencer page (BUTTON4 analog on AVR)
  if (is_press(event)) {
    b_consumed_ = false;
  } else if (is_release(event)) {
    if (!b_consumed_) {
      const PageIndex pg = mcl.currentPage();
      if (pg == GRID_PAGE) {
        // GridPage::handleEvent already implements the cur_grid swap
        // on MDX_KEY_SCALE press (resets param1.max, re-init). Post
        // that event so we get the full sequence — not just the bit
        // flip.
        key_interface.key_event(MDX_KEY_SCALE, false);
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
        GUI.putEvent(&e);
      }
    }
    b_consumed_ = false;
  }
  return true;
}

bool SpsMode::handle_trig_forward(gui_event_t *event, uint8_t trig_idx) {
  if (!latched_) return false;
  if (trig_idx >= 16) return false;

  // Trig sub-page selection requires an explicit modifier: B (TBD_BUTTON_B)
  // or TR (TBD_BUTTON_TR) held. Without a modifier, trigs fall
  // through to the active page — so on SEQ_STEP_PAGE you can keep
  // entering parameter locks (trig press = step select), on GRID_PAGE
  // you keep triggering the track, etc. Latch state alone no longer
  // claims trigs.
  const bool b_held  = BUTTON_DOWN(ButtonsClass::TBD_BUTTON_B);
  const bool tr_held = BUTTON_DOWN(ButtonsClass::TBD_BUTTON_TR);
  if (!b_held && !tr_held) return false;

  if (is_press(event)) {
    const uint8_t max_sub_pages = MD.is_spsx ? 8 : 6;
    const uint8_t page_col = trig_idx & 0x07;
    const uint8_t half = (trig_idx >= 8) ? 1 : 0;
    const uint8_t target = page_col * 2 + half;
    if (target < max_sub_pages) {
      sub_page_ = target;
      resync_from_kit();
    }
    if (b_held)  b_consumed_  = true;
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

void SpsMode::poll_page_overlay() {
  // Install the SPS overlay (a LightPage rendered on top of whatever
  // page is currently active) once TR has been held past the hold
  // threshold. The active page stays current — we just layer the
  // overlay's display + loop on top via GUI's overlay slot.
  if (!tr_pressed_) return;
  if (GUI.overlay == &sps_overlay_page) return;
  if (clock_diff(tr_press_ms_, read_clock_ms()) <= TBD_OVERLAY_HOLD_MS) return;

  if (!latched_) set_latched(true);
  GUI.setOverlay(&sps_overlay_page);
}

#else // !PLATFORM_TBD

bool SpsMode::handle_toggle_button(gui_event_t *) { return false; }
bool SpsMode::handle_func_arrow_chord(gui_event_t *) { return false; }
bool SpsMode::handle_cluster_menus(gui_event_t *) { return false; }
bool SpsMode::handle_arrow_subpage(gui_event_t *) { return false; }
bool SpsMode::handle_sps_key_tap(gui_event_t *) { return false; }
bool SpsMode::handle_trig_forward(gui_event_t *, uint8_t) { return false; }
void SpsMode::poll_encoders() {}
void SpsMode::poll_page_overlay() {}
void SpsMode::resync_from_kit() {}
bool SpsMode::active_step_lock(uint8_t, uint8_t *) const { return false; }
void SpsMode::send_param(uint8_t) {}
bool SpsMode::encoder_passthrough_page() const { return true; }
void SpsMode::set_latched(bool) {}

#endif // PLATFORM_TBD
