#include "TbdPanel.h"

#ifdef PLATFORM_TBD

#include "MCL.h"
#include "../Drivers/MD/MD.h"
#include "BankPopupPage.h"
#include "GridPage.h"
#include "GridPages.h"
#include "GUI_hardware.h"
#include "KeyInterface.h"
#include "MCLGUI.h"
#include "MidiActivePeering.h"
#include "MidiClock.h"
#include "MixerPage.h"
#include "NoteInterface.h"
#include "SeqPages.h"
#include "SeqTrackUtil.h"

TbdPanel tbd_panel;

bool TbdPanel::open_bank_popup() {
  PageIndex pg = mcl.currentPage();
  if (pg == GRID_LOAD_PAGE || pg == GRID_SAVE_PAGE ||
      pg == TEXT_INPUT_PAGE || pg == BANK_POPUP_PAGE) {
    return false;
  }
  if (pg == GRID_PAGE && grid_page.show_slot_menu) return false;

  if (grid_page.last_page == 255 && pg != GRID_PAGE) {
    grid_page.last_page = pg;
  }
  mcl.pushPage(BANK_POPUP_PAGE);
  return true;
}

bool TbdPanel::handle_bank_arrow_cycle(gui_event_t *event) {
  if (!grid_page.bank_popup) return false;

  bool group_toggle = false;
  int8_t letter_delta = 0;
  switch (event->source) {
    case ButtonsClass::FUNC_BUTTON6: // UP
    case ButtonsClass::FUNC_BUTTON8: // DOWN
      group_toggle = true; break;
    case ButtonsClass::FUNC_BUTTON7: // LEFT
      letter_delta = -1; break;
    case ButtonsClass::FUNC_BUTTON9: // RIGHT
      letter_delta = +1; break;
    default:
      return false;
  }
  // Consume release too so the arrows don't double-act via the else-branch
  // (which would transmit MDX_KEY_LEFT/etc to the MD).
  if (event->mask != EVENT_BUTTON_PRESSED) return true;

  // Any arrow press while the popup is up brings the OLED grid back.
  grid_page.bank_popup_oled_visible = true;

  uint8_t old_group = grid_page.bank / 4;
  uint8_t new_bank = group_toggle
                       ? (grid_page.bank ^ 4)
                       : (uint8_t)((grid_page.bank + 8 + letter_delta) % 8);
  if (new_bank != grid_page.bank) {
    grid_page.bank = new_bank;
    uint16_t *mask = (uint16_t *)&grid_page.row_states[0];
    mcl_gui.set_trigleds(mask[grid_page.bank], TRIGLED_EXCLUSIVENDYNAMIC);
    grid_page.send_row_led();
    uint8_t new_group = grid_page.bank / 4;
    if (new_group != old_group) {
      MD.currentBank = new_group;
      if (MD.connected) MD.press_bankgroup_button();
    }
  }
  if (MD.connected) MD.draw_bank(grid_page.bank % 4);
  return true;
}

bool TbdPanel::handle_md_ui_event(gui_event_t *event) {
  if (!MD.connected) return false;

  const bool is_press = (event->mask == EVENT_BUTTON_PRESSED);
  const bool is_release = (event->mask == EVENT_BUTTON_RELEASED);

  // ENCODER2..4 taps in SPS-latched mode trigger MD windows/actions.
  if (event->source >= ButtonsClass::ENCODER2 &&
      event->source <= ButtonsClass::ENCODER4) {
    const uint8_t idx = event->source - ButtonsClass::ENCODER2;
    static constexpr uint16_t kEncTapMaxMs = TBD_TAP_MAX_MS;
    if (is_press) return true;
    if (!Buttons.is_encoder_tap((uint8_t)(idx + 1), kEncTapMaxMs)) return true;
    if (!MD.sps_mode.is_active()) return false; // Let MCL handle normal taps.

    const bool func_held = key_interface.is_key_down(MDX_KEY_FUNC);
    switch (event->source) {
      case ButtonsClass::ENCODER2: // TEMPO
        if (func_held) MD.tap_tempo();
        else           MD.toggle_tempo_window();
        break;
      case ButtonsClass::ENCODER3: // PAT/SONG (FUNC = GLOBAL menu)
        if (func_held) {
          MD.hold_function_button();
          MD.toggle_global_window();
          MD.release_function_button();
        } else {
          MD.press_patternsong_button();
        }
        break;
      case ButtonsClass::ENCODER4: // KIT (FUNC = machine select)
        if (func_held) {
          MD.hold_function_button();
          MD.toggle_kit_menu();
          MD.release_function_button();
        } else {
          MD.toggle_kit_menu();
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
      MD.press_no_button();
      if (MD.sps_mode.is_active()) {
        key_interface.set_key_state(MDX_KEY_NO, true);
      } else {
        key_interface.key_event(MDX_KEY_NO, false);
      }
      return true;
    } else if (is_release) {
      MD.release_no_button();
      if (MD.sps_mode.is_active()) {
        key_interface.set_key_state(MDX_KEY_NO, false);
      } else {
        key_interface.key_event(MDX_KEY_NO, true);
      }
      return true;
    }
  }

  // Physical Y is MD SCALE in SPS-latched mode. The held-state bit is
  // maintained here; the MD sysex/hold behavior remains in SpsMode.
  if (event->source == ButtonsClass::BUTTON1 && MD.sps_mode.is_active()) {
    key_interface.set_key_state(MDX_KEY_SCALE, is_press);
  }

  // Physical X is MD YES in SPS-latched mode.
  if (event->source == ButtonsClass::BUTTON4 && MD.sps_mode.is_active()) {
    if (is_press) {
      MD.press_yes_button();
      key_interface.set_key_state(MDX_KEY_YES, true);
    } else if (is_release) {
      MD.release_yes_button();
      key_interface.set_key_state(MDX_KEY_YES, false);
    }
    return true;
  }

  if (MD.sps_mode.handle_toggle_button(event)) return true;
  if (MD.sps_mode.handle_cluster_menus(event)) return true;
  if (MD.sps_mode.handle_arrow_subpage(event))    return true;
  if (MD.sps_mode.handle_func_arrow_chord(event)) return true;
  if (MD.sps_mode.handle_sps_key_tap(event))      return true;

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
        !MD.sps_mode.is_active() &&
        (cur_pg == GRID_PAGE || cur_pg == SEQ_STEP_PAGE ||
         cur_pg == SEQ_PTC_PAGE || cur_pg == SEQ_EXTSTEP_PAGE);

    if (!arrows_local_only) {
      if (!is_release && key_interface.is_key_down(key)) return true;
      if (is_release) {
        switch (key) {
          case MDX_KEY_UP:    MD.release_up_arrow();    break;
          case MDX_KEY_DOWN:  MD.release_down_arrow();  break;
          case MDX_KEY_LEFT:  MD.release_left_arrow();  break;
          case MDX_KEY_RIGHT: MD.release_right_arrow(); break;
          default: break;
        }
      } else {
        switch (key) {
          case MDX_KEY_UP:    MD.hold_up_arrow();    break;
          case MDX_KEY_DOWN:  MD.hold_down_arrow();  break;
          case MDX_KEY_LEFT:  MD.hold_left_arrow();  break;
          case MDX_KEY_RIGHT: MD.hold_right_arrow(); break;
          default: break;
        }
      }

      if (MD.sps_mode.is_active()) {
        key_interface.set_key_state(key, !is_release);
        return true;
      }
    }
  }

  if (event->source >= ButtonsClass::TRIG_BUTTON1 &&
      event->source <  ButtonsClass::TRIG_BUTTON1 + 16) {
    uint8_t key = event->source - ButtonsClass::TRIG_BUTTON1;

    // FUNC held + trig in normal (non-latched) mode -> MD track select.
    if (!MD.sps_mode.is_active() &&
        key_interface.is_key_down(MDX_KEY_FUNC)) {
      if (is_press && key < NUM_MD_TRACKS) {
        MD.currentTrack = key;
        MD.track_select(key + 1);
      }
      return true;
    }

    if (MD.sps_mode.handle_trig_forward(event, key)) return true;
  }
  return false;
}

bool TbdPanel::handleEvent(gui_event_t *event) {
  if (!EVENT_BUTTON(event)) {
    return false;
  }

  const bool is_press   = (event->mask == EVENT_BUTTON_PRESSED);
  const bool is_release = !(event->mask & 1);

  // Menu/config page remap: while a MenuPage is active (system, midi
  // config, mcl config, MIDI port menus, etc.), TL replaces BUTTON1
  // (NO/exit) and TR replaces BUTTON4 (YES/enter) so the user can
  // navigate the menu without leaving the cluster.
  const uint8_t orig_src = event->source;
  {
    const PageIndex pg = mcl.currentPage();
    const bool is_menu_page =
        pg == SYSTEM_PAGE || pg == BOOT_MENU_PAGE ||
        pg == START_MENU_PAGE || pg == MIDI_CONFIG_PAGE ||
        pg == MD_CONFIG_PAGE || pg == CHAIN_CONFIG_PAGE ||
        pg == AUX_CONFIG_PAGE || pg == MCL_CONFIG_PAGE ||
        pg == MD_IMPORT_PAGE || pg == LOAD_PROJ_PAGE ||
        pg == MIDIPORT_MENU_PAGE || pg == PORT1_MENU_PAGE ||
        pg == PORT2_MENU_PAGE || pg == USBPORT_MENU_PAGE ||
        pg == MIDIPROGRAM_MENU_PAGE || pg == MIDICLOCK_MENU_PAGE ||
        pg == MIDIROUTE_MENU_PAGE || pg == MIDIMACHINEDRUM_MENU_PAGE ||
        pg == MIDIGENERIC_MENU_PAGE;
    if (is_menu_page) {
      if (orig_src == ButtonsClass::BUTTON2) {
        event->source = ButtonsClass::BUTTON1;
      } else if (orig_src == ButtonsClass::TBD_BUTTON_TR) {
        event->source = ButtonsClass::BUTTON4;
      }
    }
  }

  // TL -> TR chord opens the system config page. Asymmetric on purpose:
  // only fires when TR is the press edge while TL is already held.
  if (event->source == ButtonsClass::TBD_BUTTON_TR && is_press &&
      BUTTON_DOWN(ButtonsClass::BUTTON2)) {
    mcl.pushPage(SYSTEM_PAGE);
    midi_active_peering.dev1->mark_tr_consumed();
    midi_active_peering.dev2->mark_tr_consumed();
    return true;
  }

  if (handle_md_ui_event(event)) return true;

  if (event->source == ButtonsClass::BUTTON4 &&
      orig_src == ButtonsClass::BUTTON4) {
    const PageIndex pg = mcl.currentPage();
    if (is_press && pg == GRID_PAGE) {
      GUI.ignoreNextEvent(ButtonsClass::BUTTON4);
      mcl.setPage(GRID_LOAD_PAGE);
      return true;
    }
    if (is_press && pg == GRID_LOAD_PAGE) {
      GUI.ignoreNextEvent(ButtonsClass::BUTTON4);
      mcl.setPage(GRID_PAGE);
      return true;
    }
  }

  if (event->source == ButtonsClass::BUTTON1) {
    const PageIndex pg = mcl.currentPage();
    if (pg == GRID_PAGE && is_press) {
      GUI.ignoreNextEvent(ButtonsClass::BUTTON1);
      mcl.setPage(GRID_SAVE_PAGE);
      return true;
    }
    if (pg == GRID_SAVE_PAGE || pg == GRID_LOAD_PAGE) {
      if (is_press) {
        GUI.ignoreNextEvent(ButtonsClass::BUTTON1);
        mcl.setPage(GRID_PAGE);
      }
      return true;
    }
  }

  if (event->source == ButtonsClass::ENCODER1) {
    if (mcl.currentPage() == PAGE_SELECT_PAGE) return false;
    static constexpr uint16_t kEnc1TapMaxMs = TBD_TAP_MAX_MS;
    if (is_press) {
      return true;
    }
    if (Buttons.is_encoder_tap(0, kEnc1TapMaxMs)) {
      if (mcl.currentPage() == BANK_POPUP_PAGE) {
        bank_popup_page.close();
      } else {
        open_bank_popup();
      }
    }
    return true;
  }

  if (event->source >= ButtonsClass::ENCODER2 &&
      event->source <= ButtonsClass::ENCODER4) {
    // Fall through to mcl_handleEvent if the driver didn't consume it.
  }

  // BUTTON1..BUTTON4 are MCL local roles handled by mcl_handleEvent.
  if (event->source <= ButtonsClass::BUTTON4) {
    return false;
  }

  uint8_t key = 255;

  if (handle_bank_arrow_cycle(event)) return true;

  if (event->source >= ButtonsClass::TRIG_BUTTON1 &&
      event->source <  ButtonsClass::TRIG_BUTTON1 + 16) {
    key = event->source - ButtonsClass::TRIG_BUTTON1;

    if (mcl.currentPage() == GRID_PAGE && !grid_page.bank_popup) {
      if (is_press && key < NUM_MD_TRACKS) {
        MD.triggerTrack(key, 127);
        mixer_page.trig(key);
        if (SeqPage::recording && MidiClock.state == 2) {
          SeqTrackUtil::with_md_track(key, [](auto &t) { t.record_track(127); });
        }
      }
    }
  } else {
    const bool copy_mode = (key_interface.is_key_down(MDX_KEY_NO) ||
          key_interface.is_key_down(MDX_KEY_FUNC)) ||
         ((mcl.currentPage() == SEQ_STEP_PAGE ||
           mcl.currentPage() == SEQ_PTC_PAGE ||
           mcl.currentPage() == SEQ_EXTSTEP_PAGE) &&
          (key_interface.is_key_down(MDX_KEY_SCALE) ||
           note_interface.notes_count_on() > 0)) ||
         ((mcl.currentPage() == PERF_PAGE_0) &&
           (note_interface.notes_count_on() > 0));

    switch (event->source) {
      case ButtonsClass::FUNC_BUTTON1:
        key = copy_mode ? MDX_KEY_COPY : MDX_KEY_REC;
        break;
      case ButtonsClass::FUNC_BUTTON2:
        key = copy_mode ? MDX_KEY_CLEAR : MDX_KEY_PLAY;
        if (is_press && key == MDX_KEY_PLAY) {
          if (BUTTON_DOWN(ButtonsClass::FUNC_BUTTON1)) {
            seq_step_page.enable_record();
            if (MidiClock.state != MidiClockClass::STARTED) {
              MidiClock.handleImmediateMidiStart();
            }
            key = 255;
          } else if (MidiClock.state == MidiClockClass::PAUSED) {
             MidiClock.handleImmediateMidiContinue();
          } else if (MidiClock.state == MidiClockClass::STARTED) {
             MidiClock.handleImmediateMidiStop();
          }
        }
        break;
      case ButtonsClass::FUNC_BUTTON3:
        key = copy_mode ? MDX_KEY_PASTE : MDX_KEY_STOP;
        if (is_press && key == MDX_KEY_STOP &&
            (MidiClock.state == MidiClockClass::STARTED ||
             MidiClock.state == MidiClockClass::PAUSED)) {
          MidiClock.handleImmediateMidiStop();
        }
        break;
      case ButtonsClass::FUNC_BUTTON5:  key = MDX_KEY_FUNC;  break;
      case ButtonsClass::FUNC_BUTTON6:  key = MDX_KEY_UP;    break;
      case ButtonsClass::FUNC_BUTTON7:  key = MDX_KEY_LEFT;  break;
      case ButtonsClass::FUNC_BUTTON8:  key = MDX_KEY_DOWN;  break;
      case ButtonsClass::FUNC_BUTTON9:  key = MDX_KEY_RIGHT; break;
      case ButtonsClass::TBD_BUTTON_B: break;
      default: break;
    }
  }

  if (key != 255) {
    key_interface.key_event(key, is_release);
    return true;
  }
  return false;
}

bool tbd_handleEvent(gui_event_t *event) {
  return tbd_panel.handleEvent(event);
}

#endif // PLATFORM_TBD
