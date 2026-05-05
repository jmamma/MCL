#include "MDPanel.h"

#ifdef PLATFORM_TBD

#include "../MD.h"
#include "GUI_hardware.h"
#include "GridPages.h"
#include "KeyInterface.h"
#include "MCL.h"
#include "SeqPages.h"

bool MDPanel::handle_event(gui_event_t *event) {
  if (!md_.connected) return false;

  const bool is_press = (event->mask == EVENT_BUTTON_PRESSED);
  const bool is_release = (event->mask == EVENT_BUTTON_RELEASED);

  // ENCODER2..4 taps in SPS-latched mode trigger MD windows/actions.
  if (event->source >= ButtonsClass::ENCODER2 &&
      event->source <= ButtonsClass::ENCODER4) {
    const uint8_t idx = event->source - ButtonsClass::ENCODER2;
    static constexpr uint16_t kEncTapMaxMs = TBD_TAP_MAX_MS;
    if (is_press) return true;
    if (!Buttons.is_encoder_tap((uint8_t)(idx + 1), kEncTapMaxMs)) return true;
    if (!md_.sps_mode.is_active()) return false; // Let MCL handle normal taps.

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

  // Physical A acts as MDX_KEY_NO for the lifetime of the hold. In
  // SPS-latched mode it is device/state only, not a local MCL command.
  if (event->source == ButtonsClass::BUTTON3) {
    if (is_press) {
      md_.press_no_button();
      if (md_.sps_mode.is_active()) {
        key_interface.set_key_state(MDX_KEY_NO, true);
      } else {
        key_interface.key_event(MDX_KEY_NO, false);
      }
      return true;
    } else if (is_release) {
      md_.release_no_button();
      if (md_.sps_mode.is_active()) {
        key_interface.set_key_state(MDX_KEY_NO, false);
      } else {
        key_interface.key_event(MDX_KEY_NO, true);
      }
      return true;
    }
  }

  // Physical Y is MD SCALE in SPS-latched mode. The held-state bit is
  // maintained here; the MD sysex/hold behavior remains in SpsMode.
  if (event->source == ButtonsClass::BUTTON1 && md_.sps_mode.is_active()) {
    key_interface.set_key_state(MDX_KEY_SCALE, is_press);
  }

  // Physical X is MD YES in SPS-latched mode.
  if (event->source == ButtonsClass::BUTTON4 && md_.sps_mode.is_active()) {
    if (is_press) {
      md_.press_yes_button();
      key_interface.set_key_state(MDX_KEY_YES, true);
    } else if (is_release) {
      md_.release_yes_button();
      key_interface.set_key_state(MDX_KEY_YES, false);
    }
    return true;
  }

  if (md_.sps_mode.handle_toggle_button(event)) return true;
  if (md_.sps_mode.handle_cluster_menus(event)) return true;
  if (md_.sps_mode.handle_arrow_subpage(event))    return true;
  if (md_.sps_mode.handle_func_arrow_chord(event)) return true;
  if (md_.sps_mode.handle_sps_key_tap(event))      return true;

  const bool is_arrow = (event->source >= ButtonsClass::FUNC_BUTTON6 &&
                         event->source <= ButtonsClass::FUNC_BUTTON9);

  if (is_arrow) {
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
    const bool arrows_local_only =
        !md_.sps_mode.is_active() &&
        (cur_pg == GRID_PAGE || cur_pg == SEQ_STEP_PAGE ||
         cur_pg == SEQ_PTC_PAGE || cur_pg == SEQ_EXTSTEP_PAGE);

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

      if (md_.sps_mode.is_active()) {
        key_interface.set_key_state(key, !is_release);
        return true;
      }
    }
  }

  if (event->source >= ButtonsClass::TRIG_BUTTON1 &&
      event->source <  ButtonsClass::TRIG_BUTTON1 + 16) {
    uint8_t key = event->source - ButtonsClass::TRIG_BUTTON1;

    // FUNC held + trig in normal (non-latched) mode -> MD track select.
    if (!md_.sps_mode.is_active() && key_interface.is_key_down(MDX_KEY_FUNC)) {
      if (is_press && key < NUM_MD_TRACKS) {
        md_.currentTrack = key;
        md_.track_select(key + 1);
      }
      return true;
    }

    if (md_.sps_mode.handle_trig_forward(event, key)) return true;
  }

  return false;
}

#endif // PLATFORM_TBD
