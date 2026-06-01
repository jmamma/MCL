#include "MDPanel.h"

#ifdef PLATFORM_TBD

#include "../MD.h"
#include "CommonPages.h"
#include "GUI_hardware.h"
#include "GridIOOverlay.h"
#include "GridPages.h"
#include "KeyInterface.h"
#include "MCL.h"
#include "MCLGUI.h"
#include "MidiClock.h"
#include "NoteInterface.h"
#include "SeqPages.h"
#include "SeqTrackUtil.h"

bool MDPanel::handle_bank_arrow_cycle(gui_event_t *event) {
  if (!grid_page.bank_popup) return false;

  bool group_toggle = false;
  int8_t letter_delta = 0;
  switch (event->source) {
    case ButtonsClass::FUNC_BUTTON6: // UP
    case ButtonsClass::FUNC_BUTTON8: // DOWN
      group_toggle = true;
      break;
    case ButtonsClass::FUNC_BUTTON7: // LEFT
      letter_delta = -1;
      break;
    case ButtonsClass::FUNC_BUTTON9: // RIGHT
      letter_delta = +1;
      break;
    default:
      return false;
  }

  // Consume release too so arrows do not also transmit MDX_KEY_LEFT/etc.
  if (event->mask != EVENT_BUTTON_PRESSED) return true;

  // Any arrow press while the popup is up brings the OLED grid back.
  grid_page.bank_popup_oled_visible = true;

  uint8_t old_group = grid_page.bank / 4;
  uint8_t new_bank = group_toggle
                         ? (grid_page.bank ^ 4)
                         : (uint8_t)((grid_page.bank + 8 + letter_delta) % 8);
  if (new_bank != grid_page.bank) {
    grid_page.bank = new_bank;
    mcl_gui.set_trigleds(grid_row_bank_mask(grid_page.row_states, grid_page.bank),
                         TRIGLED_EXCLUSIVENDYNAMIC);
    grid_page.send_row_led();
    uint8_t new_group = grid_page.bank / 4;
    if (new_group != old_group) {
      md_.currentBank = new_group;
      if (md_.connected) md_.press_bankgroup_button();
    }
  }
  if (md_.connected) md_.draw_bank(grid_page.bank % 4);
  return true;
}

void MDPanel::handle_grid_trig_preview(gui_event_t *event, uint8_t trig_idx) {
  if (event->mask != EVENT_BUTTON_PRESSED) return;
  if (trig_idx >= NUM_MD_TRACKS) return;
  if (mcl.currentPage() != GRID_PAGE) return;
  if (grid_page.bank_popup || grid_io_overlay.is_active()) return;

  md_.triggerTrack(trig_idx, 127);
  mixer_page.trig(trig_idx);
  if (SeqPage::recording && MidiClock.state == MidiClockClass::STARTED) {
    SeqTrackUtil::with_md_track(trig_idx,
                                [](auto &t) { t.record_track(127); });
  }
}

bool MDPanel::handle_event(gui_event_t *event) {
  const bool md_arrow_trace =
      EVENT_BUTTON(event) &&
      event->source >= ButtonsClass::FUNC_BUTTON6 &&
      event->source <= ButtonsClass::FUNC_BUTTON9;
  if (md_arrow_trace) {
    DEBUG_PRINT("  MDPanel::handle_event arrow=");
    DEBUG_PRINT((unsigned)event->source);
    DEBUG_PRINT(" md_connected=");
    DEBUG_PRINT((unsigned)md_.connected);
    DEBUG_PRINT(" sps_active=");
    DEBUG_PRINTLN((unsigned)md_.ui.sps_mode.is_active());
  }
  if (handle_bank_arrow_cycle(event)) {
    if (md_arrow_trace) DEBUG_PRINTLN("  -> consumed by bank_arrow_cycle");
    return true;
  }
  if (!md_.connected) {
    if (md_arrow_trace) DEBUG_PRINTLN("  -> reject (md not connected)");
    return false;
  }

  const bool is_press = (event->mask == EVENT_BUTTON_PRESSED);
  const bool is_release = (event->mask == EVENT_BUTTON_RELEASED);

  md_.ui.sps_mode.observe_sps_key_chord(event);
  if (md_.ui.sps_mode.handle_cluster_menus(event)) return true;
  if (md_.ui.sps_mode.handle_func_arrow_chord(event)) return true;

  if (md_.ui.sps_mode.is_collapsed()) {
    if (md_arrow_trace) DEBUG_PRINTLN("  -> reject (sps collapsed)");
    return false;
  }

  // ENCODER2..4 taps in SPS-latched mode trigger MD windows/actions.
  if (event->source >= ButtonsClass::ENCODER2 &&
      event->source <= ButtonsClass::ENCODER4) {
    const uint8_t idx = event->source - ButtonsClass::ENCODER2;
    static constexpr uint16_t kEncTapMaxMs = ButtonsClass::TBD_TAP_MAX_MS;
    if (is_press) {
      Buttons.handle_encoder_tap((uint8_t)(idx + 1), true, kEncTapMaxMs);
      return true;
    }
    if (!Buttons.handle_encoder_tap((uint8_t)(idx + 1), false,
                                    kEncTapMaxMs)) {
      return true;
    }
    if (!md_.ui.sps_mode.is_active()) return false; // Let MCL handle normal taps.

    const bool func_held = key_interface.is_key_down(MDX_KEY_FUNC);
    switch (event->source) {
      case ButtonsClass::ENCODER2: // TEMPO
        if (func_held) md_.tap_tempo();
        else           md_.toggle_tempo_window();
        break;
      case ButtonsClass::ENCODER3: // PAT/SONG (FUNC = GLOBAL menu)
        if (func_held) {
          md_.hold_function_button();
          md_.toggle_global_window();
          md_.release_function_button();
        } else {
          md_.press_patternsong_button();
        }
        break;
      case ButtonsClass::ENCODER4: // KIT (FUNC = machine select)
        if (func_held) {
          md_.hold_function_button();
          md_.toggle_kit_menu();
          md_.release_function_button();
        } else {
          md_.toggle_kit_menu();
        }
        break;
      default: break;
    }
    return true;
  }

  if (md_.ui.sps_mode.handle_arrow_subpage(event)) {
    if (md_arrow_trace) DEBUG_PRINTLN("  -> consumed by handle_arrow_subpage");
    return true;
  }
  if (md_.ui.sps_mode.handle_sps_key_tap(event))      return true;

  const bool is_arrow = (event->source >= ButtonsClass::FUNC_BUTTON6 &&
                         event->source <= ButtonsClass::FUNC_BUTTON9);

  if (is_arrow) {
    if (md_arrow_trace) DEBUG_PRINTLN("  -> reached is_arrow mirror block");
    uint8_t key = 255;
    switch (event->source) {
      case ButtonsClass::FUNC_BUTTON6: key = MDX_KEY_UP;    break;
      case ButtonsClass::FUNC_BUTTON7: key = MDX_KEY_LEFT;  break;
      case ButtonsClass::FUNC_BUTTON8: key = MDX_KEY_DOWN;  break;
      case ButtonsClass::FUNC_BUTTON9: key = MDX_KEY_RIGHT; break;
    }

    // On grid/seq pages in normal mode, arrows are local only and fall
    // through to the key_event path. In SPS-latched mode, or on pages
    // without local arrow ownership, arrows mirror to the MD UI.
    const PageIndex cur_pg = mcl.currentPage();
    const bool step_edit_trig_held =
        md_.ui.sps_mode.is_active() && cur_pg == SEQ_STEP_PAGE &&
        note_interface.notes_count_on() > 0;
    const bool arrows_local_only =
        step_edit_trig_held ||
        (!md_.ui.sps_mode.is_active() &&
         (cur_pg == GRID_PAGE || cur_pg == SEQ_STEP_PAGE ||
          cur_pg == SEQ_PTC_PAGE || cur_pg == SEQ_EXTSTEP_PAGE));

    if (!arrows_local_only) {
      if (!is_release && key_interface.is_key_down(key)) return true;
      if (is_release) {
        switch (key) {
          case MDX_KEY_UP:    md_.release_up_arrow();    break;
          case MDX_KEY_DOWN:  md_.release_down_arrow();  break;
          case MDX_KEY_LEFT:  md_.release_left_arrow();  break;
          case MDX_KEY_RIGHT: md_.release_right_arrow(); break;
          default: break;
        }
      } else {
        switch (key) {
          case MDX_KEY_UP:    md_.hold_up_arrow();    break;
          case MDX_KEY_DOWN:  md_.hold_down_arrow();  break;
          case MDX_KEY_LEFT:  md_.hold_left_arrow();  break;
          case MDX_KEY_RIGHT: md_.hold_right_arrow(); break;
          default: break;
        }
      }

      if (md_.ui.sps_mode.is_active()) {
        key_interface.set_key_state(key, !is_release);
        return true;
      }
    }
  }

  if (event->source >= ButtonsClass::TRIG_BUTTON1 &&
      event->source <  ButtonsClass::TRIG_BUTTON1 + 16) {
    uint8_t key = event->source - ButtonsClass::TRIG_BUTTON1;

    // MD FUNC held + trig -> MD track select. In SPS mode the physical
    // FUNC button drives MDX_KEY_FUNC directly.
    if (key_interface.is_key_down(MDX_KEY_FUNC)) {
      if (is_press && key < NUM_MD_TRACKS) {
        md_.currentTrack = key;
        md_.track_select(key + 1);
        if (md_.ui.sps_mode.is_active()) md_.ui.sps_mode.resync_from_kit();
      }
      return true;
    }

    if (md_.ui.sps_mode.handle_trig_forward(event, key)) return true;
    handle_grid_trig_preview(event, key);
  }

  return false;
}

#endif // PLATFORM_TBD
