#include "TbdPanel.h"

#ifdef PLATFORM_TBD

#include "MCL.h"
#include "../Drivers/MD/MD.h"
#include "AuxPages.h"
#include "BankPopupPage.h"
#include "DeviceManager.h"
#include "GridPage.h"
#include "GridIOOverlay.h"
#include "GridPages.h"
#include "GUI_hardware.h"
#include "KeyInterface.h"
#include "MCLGUI.h"
#include "MidiClock.h"
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

bool TbdPanel::handleEvent(gui_event_t *event) {
  if (!EVENT_BUTTON(event)) {
    return false;
  }

  const bool is_press   = (event->mask == EVENT_BUTTON_PRESSED);
  const bool is_release = !(event->mask & 1);

  const uint8_t orig_src = event->source;
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

  // TL -> TR chord opens the system config page. Asymmetric on purpose:
  // only fires when TR is the press edge while TL is already held.
  if (event->source == ButtonsClass::TBD_BUTTON_TR && is_press &&
      BUTTON_DOWN(ButtonsClass::BUTTON2)) {
    device_manager.exit_ui();
    mcl.pushPage(SYSTEM_PAGE);
    return true;
  }

  if (event->source == ButtonsClass::TBD_BUTTON_TR && !is_menu_page &&
      device_manager.enter_ui(event)) {
    return true;
  }

  if (device_manager.handle_ui_event(event)) return true;

  // In normal mode, swap the physical Y/B roles on TBD:
  //   Y -> MD FUNC key path
  //   B -> legacy BUTTON3 path used by grid/seq menus
  // SPS-latched mode keeps the driver-specific Y=NO, B=FUNC mapping.
  if (!device_manager.is_ui_active()) {
    if (orig_src == ButtonsClass::BUTTON3) {
      key_interface.key_event(MDX_KEY_FUNC, is_release);
      return true;
    }
    if (orig_src == ButtonsClass::TBD_BUTTON_B) {
      event->source = ButtonsClass::BUTTON3;
    }
  }

  // Menu/config page remap: while a MenuPage is active (system, midi
  // config, mcl config, MIDI port menus, etc.), TL replaces BUTTON1
  // (NO/exit) and TR replaces BUTTON4 (YES/enter) so the user can
  // navigate the menu without leaving the cluster.
  if (is_menu_page) {
    if (orig_src == ButtonsClass::BUTTON2) {
      event->source = ButtonsClass::BUTTON1;
    } else if (orig_src == ButtonsClass::TBD_BUTTON_TR) {
      event->source = ButtonsClass::BUTTON4;
    }
  }

  if (event->source == ButtonsClass::BUTTON4 &&
      orig_src == ButtonsClass::BUTTON4) {
    if (is_press && pg == GRID_PAGE && !grid_io_overlay.is_active()) {
      GUI.ignoreNextEvent(ButtonsClass::BUTTON4);
      grid_io_overlay.begin(GridIOOverlay::MODE_LOAD);
      return true;
    }
    if (is_press && pg == GRID_LOAD_PAGE) {
      GUI.ignoreNextEvent(ButtonsClass::BUTTON4);
      mcl.setPage(GRID_PAGE);
      return true;
    }
  }

  if (event->source == ButtonsClass::BUTTON1) {
    if (pg == GRID_PAGE && is_press && !grid_io_overlay.is_active()) {
      GUI.ignoreNextEvent(ButtonsClass::BUTTON1);
      grid_io_overlay.begin(GridIOOverlay::MODE_SAVE);
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
      Buttons.handle_encoder_tap(0, true, kEnc1TapMaxMs);
      return true;
    }
    if (Buttons.handle_encoder_tap(0, false, kEnc1TapMaxMs)) {
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

    if (mcl.currentPage() == GRID_PAGE && !grid_page.bank_popup &&
        !grid_io_overlay.is_active()) {
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
      case ButtonsClass::FUNC_BUTTON5:  break;
      case ButtonsClass::FUNC_BUTTON6:  key = MDX_KEY_UP;    break;
      case ButtonsClass::FUNC_BUTTON7:  key = MDX_KEY_LEFT;  break;
      case ButtonsClass::FUNC_BUTTON8:  key = MDX_KEY_DOWN;  break;
      case ButtonsClass::FUNC_BUTTON9:  key = MDX_KEY_RIGHT; break;
      case ButtonsClass::TBD_BUTTON_B:  key = MDX_KEY_FUNC;  break;
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
