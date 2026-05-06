#include "TbdPanel.h"

#ifdef PLATFORM_TBD

#include "../Drivers/MidiDevice.h"
#include "../Drivers/TBD/TBD.h"
#include "../Drivers/TBD/TbdUiMode.h"
#include "AuxPages.h"
#include "MCL.h"
#include "BankPopupPage.h"
#include "DeviceManager.h"
#include "GridIOOverlay.h"
#include "GridPage.h"
#include "GridPages.h"
#include "GUI_hardware.h"
#include "KeyInterface.h"
#include "MCLSysConfig.h"
#include "MidiClock.h"
#include "MidiSetup.h"
#include "MCLSeq.h"
#include "NoteInterface.h"
#include "SeqPages.h"
#include "SeqTrackUtil.h"
#include "TbdTempoPage.h"

TbdPanel tbd_panel;

static bool is_tbd_menu_page(PageIndex pg) {
  return pg == SYSTEM_PAGE || pg == BOOT_MENU_PAGE ||
         pg == START_MENU_PAGE || pg == MIDI_CONFIG_PAGE ||
         pg == MD_CONFIG_PAGE || pg == CHAIN_CONFIG_PAGE ||
         pg == AUX_CONFIG_PAGE || pg == MCL_CONFIG_PAGE ||
         pg == MD_IMPORT_PAGE || pg == LOAD_PROJ_PAGE ||
         pg == MIDIDEVICE_MENU_PAGE || pg == GRIDX_MENU_PAGE ||
         pg == GRIDY_MENU_PAGE ||
         pg == MIDIPORT_MENU_PAGE || pg == PORT1_MENU_PAGE ||
         pg == PORT2_MENU_PAGE || pg == USBPORT_MENU_PAGE ||
         pg == MIDIPROGRAM_MENU_PAGE || pg == MIDICLOCK_MENU_PAGE ||
         pg == MIDIROUTE_MENU_PAGE || pg == MIDIMACHINEDRUM_MENU_PAGE ||
         pg == MIDIGENERIC_MENU_PAGE;
}

bool TbdPanel::top_left_reserved_page() const {
  PageIndex pg = mcl.currentPage();
  return is_tbd_menu_page(pg) || pg == PAGE_SELECT_PAGE ||
         pg == BANK_POPUP_PAGE || pg == TEXT_INPUT_PAGE ||
         pg == GRID_SAVE_PAGE || pg == GRID_LOAD_PAGE ||
         grid_io_overlay.is_active();
}

bool TbdPanel::enter_primary_ui(gui_event_t *event) {
  MidiDevice *primary = device_manager.primary_device();
  if (primary == &TBD) {
    if (tbd_ui_mode.is_active() &&
        tbd_ui_mode.device_idx() == TbdUiMode::SLOT_PRIMARY) {
      return true;
    }
  } else if (primary->is_ui_active()) {
    return true;
  }

  gui_event_t ui_event = *event;
  ui_event.source = ButtonsClass::BUTTON2;
  ui_event.mask = EVENT_BUTTON_RELEASED;
  if (device_manager.enter_ui(primary, &ui_event)) {
    return true;
  }
  ui_event = *event;
  ui_event.source = ButtonsClass::BUTTON2;
  ui_event.mask = EVENT_BUTTON_RELEASED;
  return device_manager.enter_ui(&ui_event);
}

bool TbdPanel::enter_secondary_ui(gui_event_t *event) {
  MidiDevice *secondary = device_manager.secondary_device();
  if (secondary == &TBD) {
    if (tbd_ui_mode.is_active() &&
        tbd_ui_mode.device_idx() == TbdUiMode::SLOT_SECONDARY) {
      return true;
    }
  } else if (secondary->is_ui_active()) {
    return true;
  }

  gui_event_t ui_event = *event;
  ui_event.source = ButtonsClass::TBD_BUTTON_TR;
  ui_event.mask = EVENT_BUTTON_PRESSED;
  if (device_manager.enter_ui(secondary, &ui_event)) {
    return true;
  }
  ui_event = *event;
  ui_event.source = ButtonsClass::TBD_BUTTON_TR;
  ui_event.mask = EVENT_BUTTON_PRESSED;
  return device_manager.enter_ui(&ui_event);
}

void TbdPanel::loop() {
}

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

bool TbdPanel::handle_grid_trig_preview(gui_event_t *event, uint8_t trig_idx) {
  if (event->mask != EVENT_BUTTON_PRESSED) return false;
  if (trig_idx >= 16) return false;
  if (device_manager.is_ui_active()) return false;
  if (mcl.currentPage() != GRID_PAGE) return false;
  if (grid_page.bank_popup || grid_io_overlay.is_active()) return false;

  MidiDevice *primary = device_manager.primary_device();
  if (primary == nullptr ||
      !primary->supports_capability(MidiDeviceCapability::MdTrigInterface)) {
    return false;
  }

  primary->triggerTrack(trig_idx, 127);
  if (mcl_cfg.grid_x_device == GRID_X_DEVICE_TBD) {
    if (trig_idx < mcl_seq.num_tbd_tracks) {
      mixer_page.disp_levels[trig_idx] = 127;
      GUI_hardware.led.set_flashled(trig_idx);
      if (SeqPage::recording && MidiClock.state == MidiClockClass::STARTED) {
        mcl_seq.tbd_tracks[trig_idx].record_track(127);
      }
    }
  } else {
    mixer_page.trig(trig_idx);
    if (SeqPage::recording && MidiClock.state == MidiClockClass::STARTED) {
      SeqTrackUtil::with_md_track(trig_idx,
                                  [](auto &track) { track.record_track(127); });
    }
  }
  return true;
}

bool TbdPanel::handleEvent(gui_event_t *event) {
  if (!EVENT_BUTTON(event)) {
    return false;
  }

  const bool is_press   = (event->mask == EVENT_BUTTON_PRESSED);
  const bool is_release = (event->mask == EVENT_BUTTON_RELEASED);

  const uint8_t orig_src = event->source;
  const PageIndex pg = mcl.currentPage();
  const bool is_menu_page = is_tbd_menu_page(pg);
  const bool grid_page_active =
      pg == GRID_PAGE && GUI.currentPage() == mcl.getPage(GRID_PAGE);

  if (suppress_sps_key_release_ &&
      orig_src == ButtonsClass::FUNC_BUTTON5 &&
      is_release) {
    suppress_sps_key_release_ = false;
    return true;
  }

  // FUNC + TBDR opens the P4 transport/SPI diagnostic overlay. FUNC on its
  // own opens tempo, so this chord must be checked before the tempo overlay
  // gets first refusal on the TBDR press.
  if (event->source == ButtonsClass::TBD_BUTTON_TR && is_press &&
      BUTTON_DOWN(ButtonsClass::FUNC_BUTTON5) &&
      (mcl_cfg.grid_x_device == GRID_X_DEVICE_TBD ||
       mcl_cfg.grid_y_device == GRID_Y_DEVICE_TBD)) {
    uint8_t diag_device_idx =
        mcl_cfg.grid_y_device == GRID_Y_DEVICE_TBD ? 1 : 0;
    device_manager.exit_ui();
    if (TBD.enter_diag_ui(diag_device_idx)) {
      suppress_sps_key_release_ = true;
      return true;
    }
  }

  const bool can_open_grid_io_overlay =
      !device_manager.is_ui_active() && !is_menu_page &&
      grid_page_active && !grid_page.show_slot_menu &&
      !grid_io_overlay.is_active();
  auto open_grid_io_overlay = [](GridIOOverlay::Mode mode,
                                 uint8_t button) {
    GUI.ignoreNextEvent(button);
    grid_io_overlay.begin(mode);
    return true;
  };

  if (can_open_grid_io_overlay && is_press) {
    if (orig_src == ButtonsClass::BUTTON4) {
      return open_grid_io_overlay(GridIOOverlay::MODE_LOAD,
                                  ButtonsClass::BUTTON4);
    }
    if (orig_src == ButtonsClass::BUTTON1) {
      return open_grid_io_overlay(GridIOOverlay::MODE_SAVE,
                                  ButtonsClass::BUTTON1);
    }
  }

  if (tbd_tempo_page.is_active() && tbd_tempo_page.handleEvent(event)) {
    return true;
  }

  if (device_manager.is_ui_active()) {
    if (orig_src == ButtonsClass::BUTTON2) {
      if (is_press) {
        enter_primary_ui(event);
      }
      return true;
    }
    if (orig_src == ButtonsClass::TBD_BUTTON_TR && is_press) {
      enter_secondary_ui(event);
      return true;
    }
  }

  // TL -> TR chord opens the system config page. Asymmetric on purpose:
  // only fires when TR is the press edge while TL is already held.
  if (event->source == ButtonsClass::TBD_BUTTON_TR && is_press &&
      BUTTON_DOWN(ButtonsClass::BUTTON2)) {
    device_manager.exit_ui();
    mcl.pushPage(SYSTEM_PAGE);
    return true;
  }

  if (orig_src == ButtonsClass::BUTTON2 && !top_left_reserved_page()) {
    if (is_press) {
      device_manager.exit_ui();
      mcl.setPage(PAGE_SELECT_PAGE);
      return true;
    }

    if (is_release) {
      return true;
    }
  }

  if (orig_src == ButtonsClass::BUTTON2 && pg == PAGE_SELECT_PAGE &&
      is_release) {
    return false;
  }

  if (!device_manager.is_ui_active() && !is_menu_page &&
      orig_src == ButtonsClass::FUNC_BUTTON5 && is_press &&
      grid_page_active && !grid_page.show_slot_menu &&
      !grid_io_overlay.is_active()) {
    bool tap = BUTTON_DOWN(ButtonsClass::BUTTON3);
    if (tap) {
      key_interface.key_event(MDX_KEY_FUNC, true);
      GUI.ignoreNextEvent(ButtonsClass::BUTTON3);
    }
    tbd_tempo_page.begin(tap);
    return true;
  }

  if (TBD.is_ui_active() && TBD.handle_ui_event(event)) {
    return true;
  }

  if (device_manager.is_ui_active() &&
      device_manager.handle_ui_event(event)) {
    return true;
  }

  if (event->source == ButtonsClass::TBD_BUTTON_TR && is_press &&
      !is_menu_page && enter_secondary_ui(event)) {
    return true;
  }

  if (event->source >= ButtonsClass::TRIG_BUTTON1 &&
      event->source < ButtonsClass::TRIG_BUTTON1 + 16 &&
      handle_grid_trig_preview(event,
                               event->source - ButtonsClass::TRIG_BUTTON1)) {
    return true;
  }

  if (device_manager.handle_ui_event(event)) return true;

  if (!device_manager.is_ui_active() && pg == SEQ_PTC_PAGE &&
      event->source >= ButtonsClass::TRIG_BUTTON1 &&
      event->source < ButtonsClass::TRIG_BUTTON1 + 16) {
    return seq_ptc_page.handle_tbd_keyboard_event(
        event->source - ButtonsClass::TRIG_BUTTON1, event->mask);
  }

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
    if (is_press && pg == GRID_LOAD_PAGE) {
      GUI.ignoreNextEvent(ButtonsClass::BUTTON4);
      mcl.setPage(GRID_PAGE);
      return true;
    }
  }

  if (event->source == ButtonsClass::BUTTON1) {
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
    static constexpr uint16_t kEnc1TapMaxMs = ButtonsClass::TBD_TAP_MAX_MS;
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

  if (event->source >= ButtonsClass::TRIG_BUTTON1 &&
      event->source <  ButtonsClass::TRIG_BUTTON1 + 16) {
    key = event->source - ButtonsClass::TRIG_BUTTON1;
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
