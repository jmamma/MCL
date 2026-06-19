#include "TBD/UI/TbdPanel.h"

#ifdef PLATFORM_TBD

#include "../../MidiDevice.h"
#include "../../MD/MD.h"
#include "../TBD.h"
#include "GUI/Pages/CommonPages.h"
#include "MCL.h"
#include "GUI/Pages/BankPopupPage.h"
#include "DeviceManager.h"
#include "GUI/Pages/Project/FileBrowserPage.h"
#include "GUI/Pages/Grid/GridIOOverlay.h"
#include "GUI/Pages/Grid/GridPage.h"
#include "GUI/Pages/Grid/GridPages.h"
#include "GUI_hardware.h"
#include "KeyInterface.h"
#include "MenuPage.h"
#include "MCLSysConfig.h"
#include "MidiClock.h"
#include "MidiSetup.h"
#include "Sequencer/MCLSeq.h"
#include "NoteInterface.h"
#include "GUI/Pages/PageSelectPage.h"
#include "GUI/Pages/Sequencer/SeqPages.h"
#include "GUI/Pages/Sequencer/SeqTrackSelectPage.h"
#include "SeqTrackUtil.h"
#include "Pages/TbdTempoPage.h"
#include "helpers.h"

TbdPanel tbd_panel;

static constexpr uint16_t kTbdUiButtonTapMaxMs = 400;
static constexpr uint8_t kTbdSlotTrackMenuButton = ButtonsClass::BUTTON3;
static constexpr uint8_t kTbdDisplaySizeButton = ButtonsClass::TBD_BUTTON_B;

static bool tbd_transport_forward_has(MidiUartClass *uart) {
  if (uart == nullptr) return false;
  if (MidiClock.uart_transport_forward1 == uart ||
      MidiClock.uart_transport_forward2 == uart ||
      MidiClock.uart_transport_forward3 == uart) {
    return true;
  }
  if (MidiClock.uart_transport_forward4 == uart) {
    return true;
  }
  return false;
}

static void tbd_forward_local_transport_to_md(uint8_t msg) {
  if (!MD.connected || MD.uart == nullptr || MD.uart == &MidiUartP4) return;
  if (tbd_transport_forward_has(MD.uart)) return;
  MD.uart->sendRealtime(msg);
}

static void tbd_handle_local_transport(uint8_t msg) {
  tbd_forward_local_transport_to_md(msg);
  switch (msg) {
  case MIDI_START:
    MidiClock.handleImmediateMidiStart();
    MidiClock.handleMidiStart();
    break;
  case MIDI_STOP:
    MidiClock.handleImmediateMidiStop();
    MidiClock.handleMidiStop();
    break;
  case MIDI_CONTINUE:
    MidiClock.handleImmediateMidiContinue();
    MidiClock.handleMidiContinue();
    break;
  }
}

static bool is_tbd_menu_page(PageIndex pg) {
  return pg == SYSTEM_PAGE || pg == BOOT_MENU_PAGE ||
         pg == START_MENU_PAGE || pg == MIDI_CONFIG_PAGE ||
         pg == MD_CONFIG_PAGE || pg == CHAIN_CONFIG_PAGE ||
         pg == AUX_CONFIG_PAGE || pg == MCL_CONFIG_PAGE ||
         pg == MD_IMPORT_PAGE ||
         pg == MIDIDEVICE_MENU_PAGE || pg == GRIDX_MENU_PAGE ||
         pg == GRIDY_MENU_PAGE ||
         pg == MIDIPORT_MENU_PAGE || pg == PORT1_MENU_PAGE ||
         pg == PORT2_MENU_PAGE || pg == USBPORT_MENU_PAGE ||
         pg == MIDIPROGRAM_MENU_PAGE || pg == MIDICLOCK_MENU_PAGE ||
         pg == MIDIROUTE_MENU_PAGE || pg == MIDIMACHINEDRUM_MENU_PAGE ||
         pg == MIDIGENERIC_MENU_PAGE ||
         pg == MIDICONTROLINPUT_MENU_PAGE ||
         pg == MIDICONTROLOUTPUT_MENU_PAGE;
}

static bool is_tbd_file_browser_page(PageIndex pg) {
  return pg == LOAD_PROJ_PAGE ||
#ifdef MCL_HAS_PROJECT_BACKUP
         pg == PROJECT_VERSION_PAGE ||
#endif
         pg == SAMPLE_BROWSER || pg == SOUND_BROWSER;
}

static FileBrowserPage *active_tbd_file_browser_page(PageIndex pg) {
  if (!is_tbd_file_browser_page(pg)) return nullptr;
  return static_cast<FileBrowserPage *>(GUI.currentPage());
}

static MenuPageBase *active_tbd_menu_page(PageIndex pg) {
  if (!is_tbd_menu_page(pg)) return nullptr;
  return static_cast<MenuPageBase *>(GUI.currentPage());
}

static bool driver_ui_blocked_page(PageIndex pg) {
  switch (pg) {
  case MIXER_PAGE:
  case PAGE_SELECT_PAGE:
  case QUESTIONDIALOG_PAGE:
  case ARP_PAGE:
  case LOAD_PROJ_PAGE:
  case SAMPLE_BROWSER:
  case SOUND_BROWSER:
    return true;
#ifdef WAV_DESIGNER
  case WD_MIXER_PAGE:
  case WD_PAGE_0:
  case WD_PAGE_1:
  case WD_PAGE_2:
    return true;
#endif
  default:
    return false;
  }
}

static MidiDevice *grid_trig_preview_device() {
  MidiDevice *primary = device_manager.primary_device();
  if (primary != nullptr &&
      primary->supports_capability(MidiDeviceCapability::MdTrigInterface)) {
    return primary;
  }

  MidiDevice *secondary = device_manager.secondary_device();
  if (secondary != nullptr && secondary != primary &&
      secondary->supports_capability(MidiDeviceCapability::MdTrigInterface)) {
    return secondary;
  }

  return nullptr;
}

static bool tbd_y_button3_page(PageIndex pg) {
  switch (pg) {
  case GRID_PAGE:
  case MIXER_PAGE:
  case GRID_SAVE_PAGE:
  case GRID_LOAD_PAGE:
    return true;
  case SEQ_STEP_PAGE:
  case SEQ_EXTSTEP_PAGE:
  case SEQ_PTC_PAGE:
    return true;
  default:
    return false;
  }
}

static bool tbd_seq_page(PageIndex pg) {
  return pg == SEQ_STEP_PAGE || pg == SEQ_EXTSTEP_PAGE || pg == SEQ_PTC_PAGE;
}

static bool tbd_b_scale_page(PageIndex pg) {
  switch (pg) {
  case GRID_PAGE:
    return GUI.currentPage() == mcl.getPage(GRID_PAGE) &&
           !grid_page.show_slot_menu;
  case MIXER_PAGE:
  case GRID_SAVE_PAGE:
  case GRID_LOAD_PAGE:
    return true;
  case SEQ_STEP_PAGE:
  case SEQ_EXTSTEP_PAGE:
  case SEQ_PTC_PAGE:
    return !SeqPage::show_seq_menu;
  default:
    return false;
  }
}

static bool tbd_bank_popup_tap_allowed(PageIndex pg) {
  if (pg == BANK_POPUP_PAGE) return true;
  if (pg == PAGE_SELECT_PAGE || pg == GRID_LOAD_PAGE ||
      pg == GRID_SAVE_PAGE || pg == TEXT_INPUT_PAGE ||
      pg == QUESTIONDIALOG_PAGE || grid_io_overlay.is_active()) {
    return false;
  }
  if (is_tbd_menu_page(pg) || is_tbd_file_browser_page(pg)) return false;
  if (pg == GRID_PAGE && grid_page.show_slot_menu) return false;
  return true;
}

static void tbd_page_select_current_page(PageIndex pg) {
  const uint8_t count = sizeof(page_select_page.page_entries) /
                        sizeof(page_select_page.page_entries[0]);
  for (uint8_t i = 0; i < count; ++i) {
    if (page_select_page.page_entries[i].Page == pg) {
      if (page_select_page.page_select != i) {
        page_select_page.page_select = i;
        page_select_page.draw_popup();
      }
      return;
    }
  }
}

bool TbdPanel::top_left_button_consumed_page(uint8_t pg) const {
  return pg != PAGE_SELECT_PAGE && pg != BANK_POPUP_PAGE &&
         pg != TEXT_INPUT_PAGE && pg != GRID_SAVE_PAGE &&
         pg != GRID_LOAD_PAGE &&
         !grid_io_overlay.is_active();
}

bool TbdPanel::handle_primary_ui_button(gui_event_t *event) {
  return device_manager.handle_ui_slot_button(DeviceManager::UI_SLOT_PRIMARY,
                                             event, false);
}

bool TbdPanel::handle_secondary_ui_button(gui_event_t *event,
                                          bool allow_toggle) {
  return device_manager.handle_ui_slot_button(DeviceManager::UI_SLOT_SECONDARY,
                                             event, allow_toggle);
}

bool TbdPanel::open_secondary_ui_from_tap(gui_event_t *event) {
  gui_event_t open_event = *event;
  open_event.mask = EVENT_BUTTON_PRESSED;
  return device_manager.enter_ui_slot_tap(DeviceManager::UI_SLOT_SECONDARY,
                                          &open_event);
}

bool TbdPanel::handle_active_ui_button(gui_event_t *event, uint8_t orig_src) {
  if (orig_src != ButtonsClass::BUTTON2 &&
      orig_src != ButtonsClass::TBD_BUTTON_TR) {
    return false;
  }

  if (event->mask == EVENT_BUTTON_PRESSED) {
    active_ui_button_pressed_ = true;
    active_ui_button_chorded_ = false;
    active_ui_button_source_ = orig_src;
    active_ui_button_press_ms_ = read_clock_ms();
    active_ui_button_exit_on_tap_ =
        orig_src == ButtonsClass::BUTTON2
            ? device_manager.is_ui_slot_active(DeviceManager::UI_SLOT_PRIMARY)
            : true;
    if (orig_src == ButtonsClass::BUTTON2) {
      handle_primary_ui_button(event);
    } else {
      device_manager.notify_active_ui_button(event);
    }
    return true;
  }

  if (event->mask == EVENT_BUTTON_RELEASED) {
    const bool tracked =
        active_ui_button_pressed_ && active_ui_button_source_ == orig_src;
    const uint16_t held_ms =
        tracked ? clock_diff(active_ui_button_press_ms_, read_clock_ms()) : 0;
    const bool chorded = active_ui_button_chorded_;
    const bool short_tap =
        tracked && !chorded && held_ms <= kTbdUiButtonTapMaxMs;

    active_ui_button_pressed_ = false;
    active_ui_button_chorded_ = false;
    active_ui_button_source_ = 255;
    const bool exit_on_tap = active_ui_button_exit_on_tap_;
    active_ui_button_exit_on_tap_ = false;

    if (orig_src == ButtonsClass::BUTTON2) {
      handle_primary_ui_button(event);
    } else {
      device_manager.notify_active_ui_button(event);
    }
    if (short_tap && exit_on_tap) {
      device_manager.exit_ui();
    }
    return true;
  }

  return true;
}

void TbdPanel::open_page_select() {
  device_manager.exit_ui();
  PageIndex current_page = mcl.currentPage();
  if (current_page != PAGE_SELECT_PAGE) {
    top_left_page_select_base_page_ = current_page;
  }
  mcl.setPage(PAGE_SELECT_PAGE);
  if (current_page != PAGE_SELECT_PAGE) {
    tbd_page_select_current_page(current_page);
  }
}

void TbdPanel::loop() {}

bool TbdPanel::open_bank_popup() {
  PageIndex pg = mcl.currentPage();
  if (!tbd_bank_popup_tap_allowed(pg) || pg == BANK_POPUP_PAGE) {
    return false;
  }

  device_manager.exit_ui();
  if (grid_page.last_page == 255) {
    grid_page.last_page = pg;
  }
  mcl.pushPage(BANK_POPUP_PAGE);
  return true;
}

bool TbdPanel::handle_grid_trig_preview(gui_event_t *event, uint8_t trig_idx) {
  if (event->mask != EVENT_BUTTON_PRESSED) return false;
  if (trig_idx >= 16) return false;
  if (mcl.currentPage() != GRID_PAGE) return false;
  if (grid_page.bank_popup || grid_io_overlay.is_active()) return false;

  MidiDevice *device = grid_trig_preview_device();
  if (device == nullptr) {
    return false;
  }

  device->triggerTrack(trig_idx, 127);
  if (device == &TBD) {
    if (trig_idx < mcl_seq.num_tbd_tracks) {
      mixer_page.track_trig(DeviceIdx::Primary, trig_idx, 127);
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

bool TbdPanel::handle_mixer_mute_yes_no(gui_event_t *event, uint8_t orig_src) {
  uint8_t key = 255;
  bool *latched = nullptr;

  if (orig_src == ButtonsClass::BUTTON4) {
    key = MDX_KEY_YES;
    latched = &mixer_yes_button_down_;
  } else if (orig_src == ButtonsClass::BUTTON1) {
    key = MDX_KEY_NO;
    latched = &mixer_no_button_down_;
  } else {
    return false;
  }

  if (event->mask == EVENT_BUTTON_RELEASED) {
    if (!*latched) return false;
    *latched = false;
    key_interface.key_event(key, true);
    return true;
  }

  if (event->mask != EVENT_BUTTON_PRESSED || mcl.currentPage() != MIXER_PAGE ||
      note_interface.notes_count_on() == 0) {
    return false;
  }

  *latched = true;
  key_interface.key_event(key, false);
  return true;
}

bool TbdPanel::handleEvent(gui_event_t *event) {
  if (!EVENT_BUTTON(event)) {
    return false;
  }

  const bool is_press   = (event->mask == EVENT_BUTTON_PRESSED);
  const bool is_release = (event->mask == EVENT_BUTTON_RELEASED);

  const uint8_t orig_src = event->source;
  const bool is_trig_button =
      orig_src >= ButtonsClass::TRIG_BUTTON1 &&
      orig_src < ButtonsClass::TRIG_BUTTON1 + 16;
  const PageIndex pg = mcl.currentPage();
  const bool is_menu_page = is_tbd_menu_page(pg);
  const bool is_file_browser_page = is_tbd_file_browser_page(pg);
  const bool is_local_nav_page = is_menu_page || is_file_browser_page;
  const bool grid_page_active =
      pg == GRID_PAGE && GUI.currentPage() == mcl.getPage(GRID_PAGE);
  const bool driver_ui_blocked = driver_ui_blocked_page(pg);
  bool ui_active = device_manager.is_ui_active();
  bool ui_collapsed = device_manager.is_ui_collapsed();
  bool ui_expanded = ui_active && !ui_collapsed;
  if (!ui_active) {
    active_ui_button_pressed_ = false;
    active_ui_button_chorded_ = false;
    active_ui_button_exit_on_tap_ = false;
    active_ui_button_source_ = 255;
  }
  if (!ui_expanded) {
    ui_b_button_held_ = false;
  }

  const bool arrow_trace = (orig_src >= ButtonsClass::FUNC_BUTTON6 &&
                            orig_src <= ButtonsClass::FUNC_BUTTON9);
  const bool is_transport_button =
      orig_src == ButtonsClass::FUNC_BUTTON1 ||
      orig_src == ButtonsClass::FUNC_BUTTON2 ||
      orig_src == ButtonsClass::FUNC_BUTTON3;
  if (arrow_trace) {
    DEBUG_PRINT("arrow ");
    DEBUG_PRINT((unsigned)orig_src);
    DEBUG_PRINT(is_press ? " P" : (is_release ? " R" : " ?"));
    DEBUG_PRINT(" pg=");
    DEBUG_PRINT((unsigned)pg);
    DEBUG_PRINT(" tbd_ui=");
    DEBUG_PRINT((unsigned)TBD.is_ui_active());
    DEBUG_PRINT(" dm_ui=");
    DEBUG_PRINT((unsigned)ui_active);
    DEBUG_PRINT(" collapsed=");
    DEBUG_PRINTLN((unsigned)ui_collapsed);
  }

  if (active_ui_button_pressed_ && is_press &&
      orig_src != active_ui_button_source_) {
    active_ui_button_chorded_ = true;
  }

  if (is_release) {
    if (orig_src == ui_display_chord_source_) {
      ui_display_chord_source_ = 255;
      return true;
    }
    if (orig_src == ui_display_chord_modifier_) {
      ui_display_chord_modifier_ = 255;
      if (active_ui_button_pressed_ && active_ui_button_source_ == orig_src) {
        device_manager.notify_active_ui_button(event);
        active_ui_button_pressed_ = false;
        active_ui_button_chorded_ = false;
        active_ui_button_exit_on_tap_ = false;
        active_ui_button_source_ = 255;
      }
      return true;
    }
  }

  if (driver_ui_blocked && ui_active) {
    device_manager.exit_ui();
    ui_active = false;
    ui_collapsed = false;
    ui_expanded = false;
  }

  if (suppress_top_left_release_ &&
      orig_src == ButtonsClass::BUTTON2 && is_release) {
    suppress_top_left_release_ = false;
    return true;
  }

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
      !driver_ui_blocked &&
      BUTTON_DOWN(ButtonsClass::FUNC_BUTTON5) &&
      (mcl_cfg.grid_x_device == GRID_X_DEVICE_TBD ||
       mcl_cfg.grid_y_device == GRID_Y_DEVICE_TBD)) {
    DeviceIdx diag_device_idx = mcl_cfg.grid_y_device == GRID_Y_DEVICE_TBD
                                     ? DeviceIdx::Secondary
                                     : DeviceIdx::Primary;
    device_manager.exit_ui();
    if (TBD.enter_diag_ui(diag_device_idx)) {
      suppress_sps_key_release_ = true;
      return true;
    }
  }

  // TL -> TR chord opens the system config page. Asymmetric on purpose:
  // only fires when TR is the press edge while TL is already held.
  if (event->source == ButtonsClass::TBD_BUTTON_TR && is_press &&
      BUTTON_DOWN(ButtonsClass::BUTTON2)) {
    device_manager.exit_ui();
    if (pg == PAGE_SELECT_PAGE && top_left_page_select_base_page_ < NUM_PAGES) {
      mcl.setPage((PageIndex)top_left_page_select_base_page_);
      top_left_page_select_base_page_ = 255;
    }
    suppress_top_left_release_ = true;
    mcl.pushPage(SYSTEM_PAGE);
    return true;
  }

  if (is_press && orig_src == kTbdDisplaySizeButton) {
    uint8_t display_chord_slot = DeviceManager::UI_SLOT_NONE;
    uint8_t display_chord_modifier = 255;
    if (BUTTON_DOWN(ButtonsClass::BUTTON2)) {
      display_chord_slot = DeviceManager::UI_SLOT_PRIMARY;
      display_chord_modifier = ButtonsClass::BUTTON2;
    } else if (BUTTON_DOWN(ButtonsClass::TBD_BUTTON_TR)) {
      display_chord_slot = DeviceManager::UI_SLOT_SECONDARY;
      display_chord_modifier = ButtonsClass::TBD_BUTTON_TR;
    }
    if (display_chord_slot != DeviceManager::UI_SLOT_NONE &&
        device_manager.toggle_ui_slot_display_mode(display_chord_slot)) {
      ui_display_chord_source_ = orig_src;
      ui_display_chord_modifier_ = display_chord_modifier;
      return true;
    }
  }

  if (EVENT_BUTTON(event) && event->source == ButtonsClass::ENCODER1) {
    static constexpr uint16_t kEnc1TapMaxMs = ButtonsClass::TBD_TAP_MAX_MS;
    if (is_press) {
      Buttons.handle_encoder_tap(0, true, kEnc1TapMaxMs);
      return true;
    }
    if (Buttons.handle_encoder_tap(0, false, kEnc1TapMaxMs)) {
      if (mcl.currentPage() == PAGE_SELECT_PAGE) {
        top_left_page_select_base_page_ = 255;
        page_select_page.close_to_selection();
      } else {
        open_page_select();
      }
    }
    return true;
  }

  if (ui_active && MD.ui.sps_mode.is_active() &&
      !is_local_nav_page && !driver_ui_blocked &&
      event->source >= ButtonsClass::ENCODER2 &&
      event->source <= ButtonsClass::ENCODER4 &&
      device_manager.handle_ui_event(event)) {
    return true;
  }

  if (event->source == ButtonsClass::ENCODER2 &&
      tbd_bank_popup_tap_allowed(pg)) {
    static constexpr uint16_t kEnc2TapMaxMs = ButtonsClass::TBD_TAP_MAX_MS;
    if (is_press) {
      Buttons.handle_encoder_tap(1, true, kEnc2TapMaxMs);
      return true;
    }
    if (Buttons.handle_encoder_tap(1, false, kEnc2TapMaxMs)) {
      if (mcl.currentPage() == BANK_POPUP_PAGE) {
        bank_popup_page.close();
      } else {
        open_bank_popup();
      }
    }
    return true;
  }

  if (ui_active && !is_local_nav_page && !driver_ui_blocked &&
      (orig_src == ButtonsClass::BUTTON2 ||
       orig_src == ButtonsClass::TBD_BUTTON_TR)) {
    return handle_active_ui_button(event, orig_src);
  }

  if (!ui_active && orig_src == ButtonsClass::BUTTON2 &&
      !is_local_nav_page && !driver_ui_blocked) {
    handle_primary_ui_button(event);
    return true;
  }

  if (orig_src == ButtonsClass::TBD_BUTTON_TR &&
      !is_local_nav_page && !driver_ui_blocked) {
    if (device_manager.is_ui_slot_active(DeviceManager::UI_SLOT_SECONDARY)) {
      if (is_press || is_release) {
        handle_secondary_ui_button(event, false);
        return !ui_collapsed;
      }
    } else if (is_press) {
      if (open_secondary_ui_from_tap(event)) {
        return true;
      }
    }
  }

  if (ui_collapsed && MD.ui.sps_mode.is_active() &&
      (orig_src == ButtonsClass::BUTTON1 ||
       orig_src == ButtonsClass::BUTTON3 ||
       orig_src == ButtonsClass::BUTTON4 ||
       orig_src == ButtonsClass::FUNC_BUTTON5 ||
       orig_src == ButtonsClass::TBD_BUTTON_B ||
       is_trig_button ||
       (orig_src >= ButtonsClass::FUNC_BUTTON6 &&
        orig_src <= ButtonsClass::FUNC_BUTTON9)) &&
      device_manager.handle_ui_event(event)) {
    return true;
  }

  if (ui_collapsed && TBD.is_ui_active() &&
      !is_local_nav_page && !driver_ui_blocked &&
      is_trig_button &&
      device_manager.handle_ui_event(event)) {
    return true;
  }

  const bool grid_select_context =
      !ui_expanded && !is_local_nav_page && grid_page_active &&
      !grid_page.show_slot_menu && !grid_io_overlay.is_active();

  if (tempo_track_select_down_ &&
      orig_src == ButtonsClass::FUNC_BUTTON5 && is_release) {
    tempo_track_select_down_ = false;
    seq_track_select_page.end();
    return true;
  }

  if (tempo_track_select_down_ && orig_src == ButtonsClass::TBD_BUTTON_B) {
    if (is_press) {
      seq_track_select_page.toggle_device();
    }
    return true;
  }

  if (tempo_track_select_down_ && orig_src == kTbdSlotTrackMenuButton) {
    if (is_press) {
      tempo_track_select_down_ = false;
      seq_track_select_page.end();
      tbd_tempo_page.begin(false);
    }
    return true;
  }

  if (tempo_track_select_down_ &&
      orig_src >= ButtonsClass::FUNC_BUTTON6 &&
      orig_src <= ButtonsClass::FUNC_BUTTON9) {
    if (is_press) {
      const int16_t delta =
          (orig_src == ButtonsClass::FUNC_BUTTON6 ||
           orig_src == ButtonsClass::FUNC_BUTTON7) ? -1 : 1;
      seq_track_select_page.move_track(delta);
    }
    return true;
  }

  if (is_trig_button && pg == GRID_PAGE &&
      seq_track_select_page.is_active()) {
    if (is_press) {
      seq_track_select_page.select_track(orig_src - ButtonsClass::TRIG_BUTTON1);
    }
    return true;
  }

  if (grid_select_button_down_ && orig_src == ButtonsClass::TBD_BUTTON_B) {
    if (is_release) {
      const bool apply_grid_select = !grid_select_button_chorded_;
      grid_select_button_down_ = false;
      grid_select_button_chorded_ = false;
      if (apply_grid_select) {
        key_interface.key_event(MDX_KEY_SCALE, false);
        key_interface.key_event(MDX_KEY_SCALE, true);
      }
    }
    return true;
  }

  if (grid_select_context && !tbd_tempo_page.is_active() &&
      orig_src == ButtonsClass::TBD_BUTTON_B) {
    if (is_press) {
      grid_select_button_down_ = true;
      grid_select_button_chorded_ = false;
    }
    return true;
  }

  if (grid_select_context && !tbd_tempo_page.is_active() &&
      orig_src == ButtonsClass::FUNC_BUTTON5) {
    if (is_press) {
      if (BUTTON_DOWN(kTbdSlotTrackMenuButton)) {
        tbd_tempo_page.begin(false);
        return true;
      }
      tempo_track_select_down_ = true;
      seq_track_select_page.begin();
      return true;
    }
    if (is_release) {
      return true;
    }
  }

  const bool can_open_grid_io_overlay =
      !ui_expanded && !is_local_nav_page &&
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
    if (arrow_trace) DEBUG_PRINTLN("  arrow eaten by tempo_page");
    return true;
  }

  if (orig_src == ButtonsClass::BUTTON2 && is_file_browser_page) {
    FileBrowserPage *browser = active_tbd_file_browser_page(pg);
    if (is_press) {
      return true;
    }
    if (is_release) {
      if (browser != nullptr) {
        if (browser->tbd_can_cd_up()) {
          return browser->tbd_cd_up();
        }
        browser->on_cancel();
        return true;
      }
    }
    return true;
  }

  if (orig_src == ButtonsClass::BUTTON2 && is_menu_page) {
    if (is_release) {
      MenuPageBase *menu = active_tbd_menu_page(pg);
      if (menu != nullptr) {
        menu->exit();
      }
    }
    return true;
  }

  if (orig_src == ButtonsClass::BUTTON2 &&
      top_left_button_consumed_page(pg)) {
    return true;
  }

  if (orig_src == ButtonsClass::BUTTON2 && pg == PAGE_SELECT_PAGE &&
      is_release) {
    top_left_page_select_base_page_ = 255;
    return false;
  }

  if (!ui_expanded && orig_src == ButtonsClass::FUNC_BUTTON5 &&
      tbd_seq_page(pg)) {
    key_interface.key_event(MDX_KEY_FUNC, is_release);
    return true;
  }

  if (TBD.is_ui_active() &&
      (!TBD.is_ui_collapsed() ||
       (orig_src >= ButtonsClass::FUNC_BUTTON6 &&
        orig_src <= ButtonsClass::FUNC_BUTTON9))) {
    if (arrow_trace) DEBUG_PRINTLN("  -> TBD.handle_ui_event");
    if (TBD.handle_ui_event(event)) {
      if (arrow_trace) DEBUG_PRINTLN("  TBD.handle_ui_event consumed");
      return true;
    }
    if (arrow_trace) DEBUG_PRINTLN("  TBD.handle_ui_event did NOT consume");
  }

  if (ui_expanded &&
      device_manager.handle_ui_event(event)) {
    if (arrow_trace) DEBUG_PRINTLN("  device_manager.handle_ui_event consumed");
    return true;
  }

  if (ui_expanded && !is_transport_button) {
    const bool tbd_ui_active = TBD.is_ui_active();
    if (!tbd_ui_active) {
      ui_b_button_held_ = false;
    }
    if (tbd_ui_active && orig_src == ButtonsClass::TBD_BUTTON_B) {
      ui_b_button_held_ = is_press;
      if (is_release) ui_b_button_held_ = false;
      return true;
    }
    if (is_trig_button) {
      if (tbd_ui_active && ui_b_button_held_ && is_press) {
        TBD.select_ui_track(orig_src - ButtonsClass::TRIG_BUTTON1);
        return true;
      }

      const uint8_t trig_idx = orig_src - ButtonsClass::TRIG_BUTTON1;
      if (pg == GRID_PAGE) {
        handle_grid_trig_preview(event, trig_idx);
        return true;
      }
      if (pg == SEQ_PTC_PAGE || pg == ARP_PAGE) {
        if (seq_ptc_page.handle_tbd_keyboard_event(trig_idx, event->mask)) {
          return true;
        }
        key_interface.key_event(trig_idx, is_release);
        return true;
      }
      if (pg == SEQ_STEP_PAGE || pg == SEQ_EXTSTEP_PAGE) {
        key_interface.key_event(trig_idx, is_release);
        return true;
      }
      return true;
    }
    return true;
  }

  // Normal MCL pages use a panel-local Y/B mapping:
  //   Grid/GridIO/Mixer/Seq: B -> Scale/page-toggle, Y -> legacy BUTTON3 menus
  // Other pages keep Y as MD FUNC and B as legacy BUTTON3. Expanded driver UI
  // handling runs before this block, so SPS fullscreen UI keeps its raw cluster
  // semantics. Collapsed driver UI intentionally follows the active page.
  if (!ui_expanded) {
    if (orig_src == ButtonsClass::BUTTON3) {
      if (!is_menu_page && tbd_y_button3_page(pg)) {
        event->source = ButtonsClass::BUTTON3;
      } else {
        key_interface.key_event(MDX_KEY_FUNC, is_release);
        return true;
      }
    }
    if (orig_src == ButtonsClass::TBD_BUTTON_B) {
      if (!is_menu_page && tbd_b_scale_page(pg)) {
        key_interface.key_event(MDX_KEY_SCALE, is_release);
        return true;
      } else {
        event->source = ButtonsClass::BUTTON3;
      }
    }
  }

  if (is_trig_button &&
      handle_grid_trig_preview(event,
                               event->source - ButtonsClass::TRIG_BUTTON1)) {
    return true;
  }

  if (handle_mixer_mute_yes_no(event, orig_src)) {
    return true;
  }

  if (!ui_collapsed && !driver_ui_blocked &&
      device_manager.handle_ui_event(event)) {
    return true;
  }

  if (!ui_expanded && (pg == SEQ_PTC_PAGE || pg == ARP_PAGE) &&
      event->source >= ButtonsClass::TRIG_BUTTON1 &&
      event->source < ButtonsClass::TRIG_BUTTON1 + 16) {
    if (seq_ptc_page.handle_tbd_keyboard_event(
            event->source - ButtonsClass::TRIG_BUTTON1, event->mask)) {
      return true;
    }
  }

  if (is_menu_page || is_file_browser_page) {
    if (orig_src == ButtonsClass::TBD_BUTTON_TR) {
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
              tbd_handle_local_transport(MIDI_START);
            }
            key = 255;
          } else if (MidiClock.state == MidiClockClass::PAUSED) {
             tbd_handle_local_transport(MIDI_CONTINUE);
          } else if (MidiClock.state == MidiClockClass::STARTED) {
             tbd_handle_local_transport(MIDI_STOP);
          }
        }
        break;
      case ButtonsClass::FUNC_BUTTON3:
        key = copy_mode ? MDX_KEY_PASTE : MDX_KEY_STOP;
        if (is_press && key == MDX_KEY_STOP &&
            (MidiClock.state == MidiClockClass::STARTED ||
             MidiClock.state == MidiClockClass::PAUSED)) {
          tbd_handle_local_transport(MIDI_STOP);
        }
        break;
      case ButtonsClass::FUNC_BUTTON5:
        if (tbd_seq_page(pg)) {
          key = MDX_KEY_FUNC;
        }
        break;
      case ButtonsClass::FUNC_BUTTON6:  key = MDX_KEY_UP;    break;
      case ButtonsClass::FUNC_BUTTON7:  key = MDX_KEY_LEFT;  break;
      case ButtonsClass::FUNC_BUTTON8:  key = MDX_KEY_DOWN;  break;
      case ButtonsClass::FUNC_BUTTON9:  key = MDX_KEY_RIGHT; break;
      case ButtonsClass::TBD_BUTTON_B:  key = MDX_KEY_FUNC;  break;
      default: break;
    }
  }

  if (key != 255) {
    if (arrow_trace) {
      DEBUG_PRINT("  arrow fell through to MDX_KEY ");
      DEBUG_PRINTLN((unsigned)key);
    }
    key_interface.key_event(key, is_release);
    return true;
  }
  if (arrow_trace) DEBUG_PRINTLN("  arrow returned false (unhandled)");
  return false;
}

bool tbd_handleEvent(gui_event_t *event) {
  return tbd_panel.handleEvent(event);
}

#endif // PLATFORM_TBD
