#include "MDPanel.h"

#ifdef PLATFORM_TBD

#include "../MD.h"
#include "GUI_hardware.h"
#include "GridPages.h"
#include "KeyInterface.h"
#include "MCL.h"
#include "NoteInterface.h"
#include "SeqPages.h"

bool MDPanel::handle_event(gui_event_t *event) {
  if (!md_.connected) return false;

  const bool is_press = (event->mask == EVENT_BUTTON_PRESSED);
  const bool is_release = (event->mask == EVENT_BUTTON_RELEASED);

  md_.sps_mode.observe_sps_key_chord(event);

  // ENCODER2..4 taps in SPS-latched mode trigger MD windows/actions.
  if (event->source >= ButtonsClass::ENCODER2 &&
      event->source <= ButtonsClass::ENCODER4) {
    const uint8_t idx = event->source - ButtonsClass::ENCODER2;
    static constexpr uint16_t kEncTapMaxMs = TBD_TAP_MAX_MS;
    if (is_press) {
      Buttons.handle_encoder_tap((uint8_t)(idx + 1), true, kEncTapMaxMs);
      return true;
    }
    if (!Buttons.handle_encoder_tap((uint8_t)(idx + 1), false,
                                    kEncTapMaxMs)) {
      return true;
    }
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

  // Physical Y is MD NO in SPS-latched mode. Normal-mode TBD routing is
  // handled by TbdPanel after the driver has seen the raw event.
  if (event->source == ButtonsClass::BUTTON3) {
    if (!md_.sps_mode.is_active()) return false;
    if (is_press) {
      md_.press_no_button();
      key_interface.set_key_state(MDX_KEY_NO, true);
      return true;
    } else if (is_release) {
      md_.release_no_button();
      key_interface.set_key_state(MDX_KEY_NO, false);
      return true;
    }
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
    const bool step_edit_trig_held =
        md_.sps_mode.is_active() && cur_pg == SEQ_STEP_PAGE &&
        note_interface.notes_count_on() > 0;
    const bool arrows_local_only =
        step_edit_trig_held ||
        (!md_.sps_mode.is_active() &&
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

      if (md_.sps_mode.is_active()) {
        key_interface.set_key_state(key, !is_release);
        return true;
      }
    }
  }

  if (event->source >= ButtonsClass::TRIG_BUTTON1 &&
      event->source <  ButtonsClass::TRIG_BUTTON1 + 16) {
    uint8_t key = event->source - ButtonsClass::TRIG_BUTTON1;

    // MD FUNC held + trig -> MD track select. Physical FUNC_BUTTON5 is
    // separate from MDX_KEY_FUNC and still drives SPS sub-page selection.
    if (key_interface.is_key_down(MDX_KEY_FUNC)) {
      if (is_press && key < NUM_MD_TRACKS) {
        md_.currentTrack = key;
        md_.track_select(key + 1);
        if (md_.sps_mode.is_active()) md_.sps_mode.resync_from_kit();
      }
      return true;
    }

    if (md_.sps_mode.handle_trig_forward(event, key)) return true;
  }

  return false;
}

#endif // PLATFORM_TBD
