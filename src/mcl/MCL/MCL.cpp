#include "MCL.h"
#include "MCLDefines.h"
#include "ResourceManager.h"
#include "MCLSd.h"
#include "MCLGfx.h"
#include "MCLGUI.h"
#include "GridTrack.h"
#include "GridTask.h"
#include "EmptyTrack.h"
#include "../Drivers/MD/UI/MDTrackSelect.h"
#include "../Drivers/MD/MD.h"
#include "../Drivers/MNM/MNM.h"
#include "../Drivers/A4/A4.h"
#include "MidiSetup.h"
#include "Project.h"
#include "MCLStrings.h"
#include "MCLSysConfig.h"


#include "GUI/Pages/Grid/GridPages.h"
#include "MidiActivePeering.h"
#include "GUI/Pages/CommonPages.h"
#include "MDPages.h"
#include "GUI/Pages/Project/ProjectPages.h"
#include "GUI/Pages/Sequencer/SeqPages.h"
#include "SeqTrackUtil.h"
#include "DeviceManager.h"

#include "GUI/Pages/PageSelectPage.h"
#include "MenuPage.h"
#include "GUI/Pages/Performance/MixerPage.h"
#include "GUI/Pages/Grid/GridSavePage.h"
#include "GUI/Pages/Grid/GridLoadPage.h"
#include "platform.h"

#ifdef WAV_DESIGNER
#include "GUI/Pages/WavDesigner/OscMixerPage.h"
#include "GUI/Pages/WavDesigner/WavDesignerPage.h"
#include "WavDesigner.h"
#endif

#include "GUI/Pages/TextInputPage.h"
#include "GUI/Pages/Sequencer/PolyPage.h"
#include "GUI/Pages/SampleBrowserPage.h"
#include "GUI/Pages/QuestionDialogPage.h"
#include "../Drivers/MD/UI/Pages/FXPage.h"
#include "../Drivers/MD/UI/Pages/RoutePage.h"
#include "GUI/Pages/Sequencer/LFOPage.h"
#include "../Drivers/MD/UI/Pages/RAMPage.h"
#include "GUI/Pages/SoundBrowserPage.h"
#include "GUI/Pages/Performance/PerfPage.h"
#ifdef MCL_HAS_EXTENDED_PANEL_INPUT
#include "GUI/Pages/PlatformPanel.h"
#endif
#ifdef PLATFORM_TBD
#include "GUI/Pages/BankPopupPage.h"
#endif

namespace {

bool remote_fill_window_open = false;
#ifdef PLATFORM_TBD
bool remote_mute_window_open = false;
bool remote_fill_scale_down = false;
#endif

uint8_t mask_shortcut_for_key(uint8_t key) {
  uint8_t bank = key - MDX_KEY_BANKA;
  if (bank > MDX_KEY_BANKD - MDX_KEY_BANKA) {
    return MASK_PATTERN;
  }
  return bank < MASK_SWING ? MASK_MUTE : bank;
}

bool bank_key_down() {
  return ((uint8_t *)&key_interface.cmd_key_state)[MDX_KEY_BANKA >> 3] & 0x3C;
}

#ifdef PLATFORM_TBD
void clear_remote_func_window() {
  remote_fill_window_open = false;
  remote_mute_window_open = false;
}
#endif

void open_remote_fill_window() {
  if (grid_page.bank_popup) {
    grid_page.close_bank_popup();
  }
  remote_fill_window_open = true;
#ifdef PLATFORM_TBD
  remote_mute_window_open = false;
#endif
}

void close_remote_fill_window() {
  remote_fill_window_open = false;
}

void toggle_remote_fill_window() {
  if (remote_fill_window_open) {
    close_remote_fill_window();
  } else {
    open_remote_fill_window();
  }
}

#ifdef PLATFORM_TBD
bool switch_remote_mute_fill_window() {
  if (!MD.connected) {
    return false;
  }
  if (remote_fill_window_open) {
    close_remote_fill_window();
    MD.toggle_mute_window();
    remote_mute_window_open = true;
    return true;
  }
  if (remote_mute_window_open) {
    open_remote_fill_window();
    return true;
  }
  return false;
}
#endif

} // namespace

#ifdef PLATFORM_TBD
void mcl_remote_func_window_replaced() {
  clear_remote_func_window();
}

void mcl_toggle_remote_mute_window() {
  if (!MD.connected) {
    return;
  }
  if (remote_fill_window_open) {
    close_remote_fill_window();
  }
  MD.toggle_mute_window();
  remote_mute_window_open = !remote_mute_window_open;
}
#endif

// In MCL.cpp:
const lightpage_ptr_t MCL::pages_table[NUM_PAGES] PROGMEM = {
    // Core pages
    { .ptr = &grid_page },
    { .ptr = &page_select_page },
    { .ptr = &system_page },
    { .ptr = &mixer_page },
    { .ptr = &grid_save_page },
    { .ptr = &grid_load_page },

    // Main sequence pages
    { .ptr = &seq_step_page },
    { .ptr = &seq_extstep_page },
    { .ptr = &seq_ptc_page },

    // UI pages
    { .ptr = &text_input_page },
    { .ptr = &poly_page },
    { .ptr = &sample_browser },
    { .ptr = &questiondialog_page },
    { .ptr = &start_menu_page },
    { .ptr = &boot_menu_page },

    // Effect pages
    { .ptr = &fx_page_a },
    { .ptr = &fx_page_b },
    { .ptr = &route_page },
    { .ptr = &lfo_page },

    // Memory pages
    { .ptr = &ram_page_a },
    { .ptr = &ram_page_b },

    // Configuration pages
    { .ptr = &load_proj_page },
#ifdef MCL_HAS_PROJECT_BACKUP
    { .ptr = &project_version_page },
#else
    { .ptr = &load_proj_page },
#endif
    { .ptr = &midi_config_page },
    { .ptr = &md_config_page },
    { .ptr = &chain_config_page },
    { .ptr = &mcl_config_page },
    { .ptr = &mcl_config_page },

    // Additional feature pages
    { .ptr = &arp_page },
    { .ptr = &md_import_page },

    // MIDI menu pages
    { .ptr = &mididevice_menu_page },
    { .ptr = &gridx_menu_page },
    { .ptr = &gridy_menu_page },
    { .ptr = &midiport_menu_page },
    { .ptr = &port1_menu_page },
    { .ptr = &port2_menu_page },
    { .ptr = &usbport_menu_page },
    { .ptr = &midiprogram_menu_page },
    { .ptr = &midiclock_menu_page },
    { .ptr = &midiroute_menu_page },
    { .ptr = &midimachinedrum_menu_page },
    { .ptr = &midigeneric_menu_page },
    { .ptr = &midicontrolinput_menu_page },
    { .ptr = &midicontroloutput_menu_page },

    // Browser pages
    { .ptr = &sound_browser },

    // Performance page
    { .ptr = &perf_page },

#ifdef PLATFORM_TBD
    { .ptr = &bank_popup_page },
#endif
#ifdef WAV_DESIGNER
    // WAV Designer pages
    { .ptr = &wd.mixer },
    { .ptr = &wd.pages[0] },
    { .ptr = &wd.pages[1] },
    { .ptr = &wd.pages[2] },
#endif
};

void mcl_setup() { mcl.current_page = NULL_PAGE; } //Exit blocking loop in mcl_setup

void MCL::setup() {

  DEBUG_PRINTLN(F("Welcome to MegaCommand Live"));
  DEBUG_PRINTLN(VERSION);

  DEBUG_DUMP(sizeof(MDTrack));
  DEBUG_DUMP(sizeof(A4Track));
  DEBUG_DUMP(sizeof(ExtTrack));
  DEBUG_DUMP(sizeof(EmptyTrack));

  DEBUG_DUMP(sizeof(MDRouteTrack));
  DEBUG_DUMP(sizeof(MDFXTrack));
  DEBUG_DUMP(sizeof(MDTempoTrack));
  DEBUG_DUMP(sizeof(GridChainTrack));

  GUI.init();

  bool ret = false;

  delay(50);
#ifdef HEALTHCHECK
  health_status health = health_check();
  if (health != HEALTH_OK) {
    setLed();
    setLed2();
    char value_str[4];
    mcl_gui.put_value_at(health, value_str);
    oled_display.init_display();
    // mclstr_hw_error is PROGMEM, value_str is RAM. AVR's Adafruit_SSD1305
    // only exposes textbox(RAM, RAM) and rp2040's Oled has no mixed overload,
    // so copy the PROGMEM label into RAM first.
    char hw_err[17];
    strncpy_P(hw_err, mclstr_hw_error, sizeof(hw_err));
    hw_err[sizeof(hw_err) - 1] = '\0';
    oled_display.textbox(hw_err, value_str);
    oled_display.display();
    while (1);
  }
#endif
  ret = mcl_sd.sd_init();
  oled_display.init_display();

  if (BUTTON_DOWN(Buttons.BUTTON2)) {
    // gfx.draw_evil(R.icons_boot->evilknievel_bitmap);
    mcl.setPage(BOOT_MENU_PAGE);
    while (mcl.currentPage() == BOOT_MENU_PAGE) {
      loop();
    }
  }
  if (ret) {
    R.Clear();
    R.use_icons_boot();
    gfx.splashscreen(R.icons_boot->mcl_logo_bitmap);

    ret = mcl_sd.load_init();
  }

  if (!ret) {
    oled_display.clearDisplay();
    oled_display.setFont();
    oled_display.setTextColor(WHITE, BLACK);
    oled_display.setCursor(0, 0);
    mcl_print_P(mclstr_sd_card_error);
    oled_display.display();
    delay(2000);
    return;
  }

  load_persistent_resources();

  // Platform panel input runs from GuiClass::handleTopEvent so it can
  // preempt the active page on cluster / physical-button overrides.
  GUI.addEventHandler((event_handler_t)&mcl_handleEvent);

  mcl.setPage(GRID_PAGE);

  DEBUG_PRINTLN(F("tempo:"));
  DEBUG_PRINTLN(mcl_cfg.tempo);
  MidiClock.setTempo(mcl_cfg.tempo);

  note_interface.setup();

  configure_driver_ports();

  mcl_actions.setup();
  mcl_seq.setup();

  key_interface.enable_listener();
  perf_page.setup();

  grid_task.init();

  GUI.addTask(&grid_task);
  read_clock_ms() = 0;
  GUI.addTask(&midi_active_peering);

  uint8_t boot = true;
  midi_setup.cfg_ports(boot);

  if (mcl_cfg.display_mirror == 1) {
#ifndef DEBUGMODE
    oled_display.textbox_P(mclstr_display_mirror, mclstr_mirror);
    GUI.display_mirror = true;
#endif
  }
  param4.cur = 4;
}

void MCL::loop() {
  platform_poll();

#ifndef __AVR__
  MidiUartUSB.service_background();
#endif

#ifdef PLATFORM_TBD
  MidiUartP4.service_background();
#endif

  perf_page.encoder_check();

#ifdef PLATFORM_TBD
  device_manager.ui_loop();
#endif

  key_interface.check_key_throttle();
  GUI.loop();

#ifdef MCL_HAS_EXTENDED_PANEL_INPUT
  platform_panel.loop();
#endif
}

bool mcl_handleEvent(gui_event_t *event) {
  if (EVENT_CMD(event)) {
    uint8_t key = event->source;
    PageIndex current_page = mcl.currentPage();
    if (key == MDX_KEY_FUNC && event->mask == EVENT_BUTTON_RELEASED) {
      seq_step_page.clear_mask_shortcut_suppress();
    }
    if (event->mask == EVENT_BUTTON_RELEASED) {
      if (mask_shortcut_for_key(key) != MASK_PATTERN) {
        seq_step_page.clear_mask_shortcut_suppress();
      }
    }
    if (event->mask == EVENT_BUTTON_PRESSED) {
      if (key != MDX_KEY_FUNC && key != MDX_KEY_COPY && key != MDX_KEY_CLEAR &&
          key != MDX_KEY_PASTE && key != MDX_KEY_SCALE) {
        reset_undo();
      }
      switch (key) {
        /*
              case MDX_KEY_UP:
              case MDX_KEY_DOWN:
              case MDX_KEY_LEFT:
              case MDX_KEY_RIGHT: {
                key_interface_task.setup();
                GUI.addTask(&key_interface_task);
                //return false to allow other gui handler to pick up.
                return false;
              }
        */
      case MDX_KEY_EXTENDED: {
        if (MidiClock.state == 2 && current_page != MIXER_PAGE) {
          mixer_page.last_page = current_page;
          mcl.setPage(MIXER_PAGE);
          mixer_page.ext_key_down = 1;
          mixer_page.mute_toggle = 1;
          return true;
        }
        break;
      }
      case MDX_KEY_SCALE: {
        if (!key_interface.event_func_down(event)) {
          if (bank_key_down()) {
            toggle_remote_fill_window();
#ifdef PLATFORM_TBD
            remote_fill_scale_down = true;
#endif
            return true;
          }
#ifdef PLATFORM_TBD
          if (switch_remote_mute_fill_window()) {
            remote_fill_scale_down = true;
            return true;
          }
#endif
        }
        break;
      }
      case MDX_KEY_MUTE:
      case MDX_KEY_BANKA:
      case MDX_KEY_BANKB:
      case MDX_KEY_BANKC:
      case MDX_KEY_BANKD: {
        bool func_down = key_interface.event_func_down(event);
        uint8_t shortcut_mask = mask_shortcut_for_key(key);
        if (func_down && shortcut_mask != MASK_PATTERN) {
          if (seq_step_page.should_suppress_mask_shortcut(shortcut_mask)) {
            return true;
          }
          if (current_page != SEQ_STEP_PAGE) {
            seq_step_page.prepare = true;
            seq_step_page.return_to_grid_on_mask_close = true;
            if (current_page != SOUND_BROWSER &&
                current_page != ARP_PAGE &&
                current_page != POLY_PAGE) {
              seq_step_page.last_page = current_page;
            }
            SeqPage::mask_type = shortcut_mask;
            mcl.setPage(SEQ_STEP_PAGE);
          }
          return true;
        }
        if (key == MDX_KEY_MUTE) {
          return false;
        }
        if (current_page == GRID_LOAD_PAGE ||
            current_page == GRID_SAVE_PAGE ||
            (current_page == GRID_PAGE && grid_page.show_slot_menu)) { // ||
//            (current_page == MIXER_PAGE && mixer_page.preview_mute_set != 255))
          return false;
        }
        if (func_down) {
          return false;
        }
        if (grid_page.last_page == 255) {
          grid_page.last_page = current_page;
        }
        mcl.setPage(GRID_PAGE);
        grid_page.bank_popup = 1;
        grid_page.bank_popup_loadmask = 0;
        bool clear_states = false;
        key_interface.on(clear_states);
        grid_page.bank = key - MDX_KEY_BANKA + MD.currentBank * 4;
        mcl_gui.set_trigleds(grid_row_bank_mask(grid_page.row_states, grid_page.bank),
                             TRIGLED_EXCLUSIVENDYNAMIC);

        grid_page.send_row_led();

        // uint8_t row = grid_page.bank * 16;
        // grid_page.jump_to_row(row);
        return true;
      }
      case MDX_KEY_NO: {
        if (current_page != SEQ_STEP_PAGE && SeqPage::mask_type == MASK_SWING) {
          SeqPage::mask_type = MASK_PATTERN;
          seq_step_page.config_mask_info(false);
          return true;
        }
        break;
      }
      case MDX_KEY_BANKGROUP: {
        if (current_page != TEXT_INPUT_PAGE &&
            current_page != GRID_SAVE_PAGE &&
            current_page != GRID_LOAD_PAGE &&
            !key_interface.is_key_down(MDX_KEY_PATSONG)) {
          mcl.setPage(PAGE_SELECT_PAGE);
          return true;
        }
        return false;
      }
      case MDX_KEY_REC: {
        if (current_page != SEQ_STEP_PAGE &&
            current_page != SEQ_PTC_PAGE &&
            current_page != SEQ_EXTSTEP_PAGE) {
          seq_step_page.prepare = true;
          if (current_page != SOUND_BROWSER &&
              current_page != ARP_PAGE &&
              current_page != POLY_PAGE) {
            seq_step_page.last_page = current_page;
          }
          SeqPage::mask_type = MASK_PATTERN;
          mcl.setPage(SEQ_STEP_PAGE);
        } else {
          if (SeqPage::recording) {
            seq_step_page.disable_record();
            key_interface.ignoreNextEvent(MDX_KEY_REC);
          } else {
            if (current_page == SEQ_STEP_PAGE) {
              key_interface.ignoreNextEvent(MDX_KEY_REC);
              mcl.setPage(seq_step_page.last_page);
            }
          }
        }
        return true;
      }
      case MDX_KEY_REALTIME: {
        mcl_seq.set_fill(true);
        seq_step_page.bootstrap_record();
        return true;
      }
      case MDX_KEY_COPY:
      case MDX_KEY_PASTE: {
        if (current_page == SEQ_STEP_PAGE || current_page == PERF_PAGE_0)
          break;
        if (current_page != SEQ_PTC_PAGE &&
            (key_interface.is_key_down(MDX_KEY_SCALE) ||
             key_interface.is_key_down(MDX_KEY_NO))) {
          // Ignore scale + copy/paste if page != seq_step_page
          break;
        }
        uint8_t opt = 2;
        if (current_page == SEQ_PTC_PAGE ||
            current_page == SEQ_EXTSTEP_PAGE) {
          opt = SeqPage::recording ? 2 : 1;
        }
        else {
          opt_midi_device_capture = &MD;
        }
        if (key == MDX_KEY_COPY) {
          opt_copy = opt;
          opt_copy_track_handler_cb();
        } else {
          opt_paste = opt;
          reset_undo();
          opt_paste_track_handler();
        }
        break;
      }
      case MDX_KEY_CLEAR: {
        if (current_page == SEQ_STEP_PAGE || current_page == PERF_PAGE_0)
          break;
        if ((note_interface.notes_count_on() > 0) ||
            (key_interface.is_key_down(MDX_KEY_SCALE) ||
             key_interface.is_key_down(MDX_KEY_NO)))
          break;
        opt_clear = 2;
        //  MidiDevice *dev = device_manager.secondary_device();
        if (current_page == SEQ_PTC_PAGE) { opt_clear = 1; }
        else if (current_page == SEQ_EXTSTEP_PAGE) {
          opt_clear = 1;
          if (seq_extstep_page.pianoroll_mode > 0) { opt_clear_locks_handler(); break; }
        }
        else {
          opt_midi_device_capture = &MD;
        }
        opt_clear_track_handler();
        break;
      }
      case MDX_KEY_STOP: {
        grid_task.stop_hard_callback = true;
        break;
      }
      case MDX_KEY_FUNCEXTENDED: {
        key_interface.ignoreNextEvent(MDX_KEY_EXTENDED);
        MD.restore_kit_params();
        break;
      }
      }
    }

    if (event->mask == EVENT_BUTTON_RELEASED) {
      switch (key) {
       case MDX_KEY_REC: {
        if (!SeqPage::recording && (current_page == SEQ_PTC_PAGE ||
                                    current_page == SEQ_EXTSTEP_PAGE)) {
            seq_step_page.prepare = true;
            seq_step_page.last_page = current_page;
            mcl.setPage(SEQ_STEP_PAGE);
          return true;
        }
        break;
      }
      case MDX_KEY_REALTIME: {
        mcl_seq.set_fill(false);
        return true;
      }
      case MDX_KEY_SCALE: {
#ifdef PLATFORM_TBD
        if (remote_fill_scale_down) {
          remote_fill_scale_down = false;
          return true;
        }
#endif
        if (bank_key_down()) {
          return true;
        }
        break;
      }
      case MDX_KEY_FUNCEXTENDED: {
        key_interface.ignoreNextEventClear(MDX_KEY_EXTENDED);
        return true;
      }
      }
    }
  }

  if (EVENT_BUTTON(event)) {
    if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
      mcl.setPage(PAGE_SELECT_PAGE);
      return true;
    }
  }
  return false;
}


__attribute__((weak)) health_status health_check() {
  return HEALTH_OK;
}

void sdcard_bench() {

  EmptyTrack empty_track;
  while (1) {
#ifdef DEBUGMODE
    uint16_t cl = read_clock_ms();
#endif
    for (uint8_t n = 0; n < 16; n++) {
      auto *ptrack = empty_track.load_from_grid_512(n, 0);
      ptrack->init_track_type(MD_TRACK_TYPE);
      USE_LOCK();
      SET_LOCK();
      if (ptrack) ptrack->store_in_mem(0);
      CLEAR_LOCK();
    }
    for (uint8_t n = 16; n < 32; n++) {
      auto *ptrack = empty_track.load_from_grid_512(n, 0);
      ptrack->init_track_type(A4_TRACK_TYPE);
      USE_LOCK();
      SET_LOCK();
      if (ptrack) ptrack->store_in_mem(0);
      CLEAR_LOCK();
    }
#ifdef DEBUGMODE
    uint16_t diff = clock_diff(cl, read_clock_ms());
    DEBUG_PRINT("Clock :");
    DEBUG_PRINTLN(diff);
#endif
  }
}


void MCL::load_persistent_resources() {
#if !defined(__AVR__)
  R.Clear();
  R.use_machine_param_names();
  R.use_machine_names_short();
#if defined(PLATFORM_TBD)
  R.use_icons_knob();
#endif
  R.SetPersistent();
#endif
}

MCL mcl;
