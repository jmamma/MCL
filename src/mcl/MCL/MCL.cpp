#include "mcl.h"
#include "ResourceManager.h"
#include "MCLSd.h"
#include "MCLGFX.h"
#include "MCLGUI.h"
#include "GridTrack.h"
#include "GridTask.h"
#include "EmptyTrack.h"
#include "MDTrackSelect.h"
#include "MD.h"
#include "MNM.h"
#include "A4.h"
#include "MidiSetup.h"
#include "Project.h"


#include "GridPages.h"
#include "AuxPages.h"
#include "ProjectPages.h"
#include "SeqPages.h"

#include "PageSelectPage.h"
#include "MenuPage.h"
#include "MixerPage.h"
#include "GridSavePage.h"
#include "GridLoadPage.h"

#ifdef WAV_DESIGNER
#include "OscMixerPage.h"
#include "WavDesignerPage.h"
#include "WavDesigner.h"
#endif

#include "TextInputPage.h"
#include "PolyPage.h"
#include "SampleBrowserPage.h"
#include "QuestionDialogPage.h"
#include "FXPage.h"
#include "RoutePage.h"
#include "LFOPage.h"
#include "RAMPage.h"
#include "SoundBrowserPage.h"
#include "PerfPage.h"

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
    { .ptr = &midi_config_page },
    { .ptr = &md_config_page },
    { .ptr = &chain_config_page },
    { .ptr = &aux_config_page },
    { .ptr = &mcl_config_page },

    // Additional feature pages
    { .ptr = &arp_page },
    { .ptr = &md_import_page },

    // MIDI menu pages
    { .ptr = &midiport_menu_page },
    { .ptr = &midiprogram_menu_page },
    { .ptr = &midiclock_menu_page },
    { .ptr = &midiroute_menu_page },
    { .ptr = &midimachinedrum_menu_page },
    { .ptr = &midigeneric_menu_page },

    // Browser pages
    { .ptr = &sound_browser },

    // Performance page
    { .ptr = &perf_page },

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

  DEBUG_DUMP(sizeof(MDLFOTrack));
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
    oled_display.textbox("HW ERROR:", value_str);
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
  if (!ret) {
    oled_display.print(F("SD CARD ERROR :-("));
    oled_display.display();
    delay(2000);
    return;
  }
  R.Clear();
  R.use_icons_boot();
  gfx.splashscreen(R.icons_boot->mcl_logo_bitmap);

  ret = mcl_sd.load_init();
#ifdef PLATFORM_TBD
  GUI.addEventHandler((event_handler_t)&tbd_handleEvent);
#endif
  GUI.addEventHandler((event_handler_t)&mcl_handleEvent);

  if (ret) {
    mcl.setPage(GRID_PAGE);
  }

  DEBUG_PRINTLN(F("tempo:"));
  DEBUG_PRINTLN(mcl_cfg.tempo);
  MidiClock.setTempo(mcl_cfg.tempo);

  note_interface.setup();

  MD.midi_events.enable_live_kit_update();

  mcl_actions.setup();
  mcl_seq.setup();

  MDSysexListener.setup(&Midi);

  key_interface.setup(&Midi);
  key_interface.enable_listener();

  md_track_select.setup(&Midi);

#ifdef EXT_TRACKS
  A4SysexListener.setup(&Midi2);
  MNMSysexListener.setup(&Midi2);
#endif
  perf_page.setup();

  grid_task.init();

  GUI.addTask(&grid_task);
  read_clock_ms() = 0;
  GUI.addTask(&midi_active_peering);

  uint8_t boot = true;
  midi_setup.cfg_ports(boot);

  if (mcl_cfg.display_mirror == 1) {
#ifndef DEBUGMODE
    oled_display.textbox("DISPLAY ", "MIRROR");
    GUI.display_mirror = true;
#endif
  }
  param4.cur = 4;
}

void MCL::loop() {
  perf_page.encoder_check();
  key_interface.check_key_throttle();
  GUI.loop();
}

bool tbd_handleEvent(gui_event_t *event) {
    // If button press is greater than 4, then we need to remap these as CMD
    if (EVENT_BUTTON(event) && event->source > ButtonsClass::BUTTON4) {
        uint8_t key = 255;
        // Handle trigger buttons with range check
        if (event->source >= ButtonsClass::TRIG_BUTTON1) {
            key = event->source - ButtonsClass::TRIG_BUTTON1; // MDX_KEY_TRIG1
        }
        else {
           bool copy_mode = (key_interface.is_key_down(MDX_KEY_NO) ||
                  key_interface.is_key_down(MDX_KEY_FUNC)) ||
                 ((mcl.currentPage() == SEQ_STEP_PAGE ||
                   mcl.currentPage() == SEQ_PTC_PAGE ||
                   mcl.currentPage() == SEQ_EXTSTEP_PAGE) &&
                  (key_interface.is_key_down(MDX_KEY_SCALE) ||
                   note_interface.notes_count_on() > 0)) ||
                 ((mcl.currentPage() == PERF_PAGE_0) &&
                   (note_interface.notes_count_on() > 0));
           // Handle function buttons with switch statement
            switch (event->source) {
                case ButtonsClass::FUNC_BUTTON1:
                    key = copy_mode ? MDX_KEY_COPY : MDX_KEY_REC;
                    break;
                case ButtonsClass::FUNC_BUTTON2:
                    key = copy_mode ? MDX_KEY_CLEAR : MDX_KEY_PLAY;
                    if (event->mask == EVENT_BUTTON_PRESSED) {
                      if (key == MDX_KEY_PLAY) {
                         if (MidiClock.state == MidiClockClass::PAUSED) {
                           MidiClock.handleImmediateMidiStart();
                         }
                         else if (MidiClock.state == MidiClockClass::STARTED) {
                           MidiClock.handleImmediateMidiStop();
                         }
                      }
                    }
                    break;
                case ButtonsClass::FUNC_BUTTON3:
                    key = copy_mode ? MDX_KEY_PASTE : MDX_KEY_STOP;
                    if (event->mask == EVENT_BUTTON_PRESSED) {
                      if (key == MDX_KEY_STOP) {
                         MidiClock.handleImmediateMidiStop();
                      }
                    }
                    break;
                case ButtonsClass::FUNC_BUTTON4:
                    key = MDX_KEY_YES;
                    break;
                case ButtonsClass::FUNC_BUTTON5:
                    key = MDX_KEY_NO;
                    break;
                case ButtonsClass::FUNC_BUTTON6:
                    key = MDX_KEY_UP;
                    break;
                case ButtonsClass::FUNC_BUTTON7:
                    key = MDX_KEY_LEFT;
                    break;
                case ButtonsClass::FUNC_BUTTON8:
                    key = MDX_KEY_DOWN;
                    break;
                case ButtonsClass::FUNC_BUTTON9:
                    key = MDX_KEY_RIGHT;
                    break;
                default:
                    break;
            }
        }
        if (key != 255) {
            key_interface.key_event(key, !(event->mask & 1));
            return true;
        }
    }
    return false;
}

bool mcl_handleEvent(gui_event_t *event) {
  /*
  if (EVENT_NOTE(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    MidiDevice *device = midi_active_peering.get_device(port);

    uint8_t track = event->source;
    if (device != &MD) {
      return true;
    }
  }
  */
  if (EVENT_CMD(event)) {
    uint8_t key = event->source;
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
        if (MidiClock.state == 2 && mcl.currentPage() != MIXER_PAGE) {
          mixer_page.last_page = mcl.currentPage();
          mcl.setPage(MIXER_PAGE);
          mixer_page.ext_key_down = 1;
          mixer_page.mute_toggle = 1;
          return true;
        }
        break;
      }
      case MDX_KEY_BANKA:
      case MDX_KEY_BANKB:
      case MDX_KEY_BANKC:
      case MDX_KEY_BANKD: {
        if (mcl.currentPage() == GRID_LOAD_PAGE ||
            mcl.currentPage() == GRID_SAVE_PAGE ||
            (mcl.currentPage() == GRID_PAGE && grid_page.show_slot_menu)) { // ||
//            (mcl.currentPage() == MIXER_PAGE && mixer_page.preview_mute_set != 255))
          return false;
        }
        if (key_interface.is_key_down(MDX_KEY_FUNC)) {
          return false;
        }
        if (grid_page.last_page == 255) {
          grid_page.last_page = mcl.currentPage();
        }
        mcl.setPage(GRID_PAGE);
        grid_page.bank_popup = 1;
        grid_page.bank_popup_loadmask = 0;
        bool clear_states = false;
        key_interface.on(clear_states);
        grid_page.bank = key - MDX_KEY_BANKA + MD.currentBank * 4;
        uint16_t *mask = (uint16_t *)&grid_page.row_states[0];
        mcl_gui.set_trigleds(mask[grid_page.bank], TRIGLED_EXCLUSIVENDYNAMIC);

        grid_page.send_row_led();

        // uint8_t row = grid_page.bank * 16;
        // grid_page.jump_to_row(row);
        return true;
      }
      case MDX_KEY_BANKGROUP: {
        if (mcl.currentPage() != TEXT_INPUT_PAGE &&
            mcl.currentPage() != GRID_SAVE_PAGE &&
            mcl.currentPage() != GRID_LOAD_PAGE &&
            !key_interface.is_key_down(MDX_KEY_PATSONG)) {
          mcl.setPage(PAGE_SELECT_PAGE);
          return true;
        }
        return false;
      }
      case MDX_KEY_REC: {
       if (mcl.currentPage() != SEQ_STEP_PAGE &&
          mcl.currentPage() != SEQ_PTC_PAGE &&
          mcl.currentPage() != SEQ_EXTSTEP_PAGE) {
          seq_step_page.prepare = true;
          if (mcl.currentPage() != SOUND_BROWSER && mcl.currentPage() != ARP_PAGE && mcl.currentPage() != POLY_PAGE) {
            seq_step_page.last_page = mcl.currentPage();
          }
          mcl.setPage(SEQ_STEP_PAGE);
        } else {
          if (seq_step_page.recording) {
            seq_step_page.recording = 0;
            MD.set_rec_mode(mcl.currentPage() == SEQ_STEP_PAGE);
            clearLed2();
            key_interface.ignoreNextEvent(MDX_KEY_REC);
          } else {
            if (mcl.currentPage() == SEQ_STEP_PAGE) {
              key_interface.ignoreNextEvent(MDX_KEY_REC);
              mcl.setPage(seq_step_page.last_page);
            }
          }
        }
        return true;
      }
      case MDX_KEY_REALTIME: {
        seq_step_page.bootstrap_record();
        return true;
      }
      case MDX_KEY_COPY: {
        if (mcl.currentPage() == SEQ_STEP_PAGE || mcl.currentPage() == PERF_PAGE_0)
          break;
        if (mcl.currentPage() != SEQ_PTC_PAGE &&
            (key_interface.is_key_down(MDX_KEY_SCALE) ||
             key_interface.is_key_down(MDX_KEY_NO))) {
          // Ignore scale + copy if page != seq_step_page
          break;
        }
        opt_copy = 2;
        if (mcl.currentPage() == SEQ_PTC_PAGE ||
            mcl.currentPage() == SEQ_EXTSTEP_PAGE) {
          opt_copy = SeqPage::recording ? 2 : 1;
        }
        else {
          opt_midi_device_capture = &MD;
        }
        opt_copy_track_handler_cb();
        break;
      }
      case MDX_KEY_PASTE: {
        if (mcl.currentPage() == SEQ_STEP_PAGE || mcl.currentPage() == PERF_PAGE_0)
          break;
        if (mcl.currentPage() != SEQ_PTC_PAGE &&
            (key_interface.is_key_down(MDX_KEY_SCALE) ||
             key_interface.is_key_down(MDX_KEY_NO))) {
          // Ignore scale + copy if page != seq_step_page
          break;
        }
        opt_paste = 2;
        if (mcl.currentPage() == SEQ_PTC_PAGE ||
            mcl.currentPage() == SEQ_EXTSTEP_PAGE) {
          opt_paste = SeqPage::recording ? 2 : 1;
        }
        else {
          opt_midi_device_capture = &MD;
        }
        reset_undo();
        opt_paste_track_handler();
        break;
      }
      case MDX_KEY_CLEAR: {
        if (mcl.currentPage() == SEQ_STEP_PAGE || mcl.currentPage() == PERF_PAGE_0)
          break;
        if ((note_interface.notes_count_on() > 0) ||
            (key_interface.is_key_down(MDX_KEY_SCALE) ||
             key_interface.is_key_down(MDX_KEY_NO)))
          break;
        opt_clear = 2;
        //  MidiDevice *dev = midi_active_peering.get_device(UART2_PORT);
        if (mcl.currentPage() == SEQ_PTC_PAGE) { opt_clear = 1; }
        else if (mcl.currentPage() == SEQ_EXTSTEP_PAGE) {
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
        if (!SeqPage::recording && (mcl.currentPage() == SEQ_PTC_PAGE ||
                                    mcl.currentPage() == SEQ_EXTSTEP_PAGE)) {
            seq_step_page.prepare = true;
            seq_step_page.last_page = mcl.currentPage();
            mcl.setPage(SEQ_STEP_PAGE);
          return true;
        }
      }
      case MDX_KEY_REALTIME: {
        return true;
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
  DeviceTrack *ptrack;
  while (1) {
    uint16_t cl = read_clock_ms();
    proj.select_grid(0);
    for (uint8_t n = 0; n < 16; n++) {
      auto *ptrack = empty_track.load_from_grid_512(n, 0);
      ptrack->init_track_type(MD_TRACK_TYPE);
      USE_LOCK();
      SET_LOCK();
      if (ptrack) ptrack->store_in_mem(0);
      CLEAR_LOCK();
    }
    proj.select_grid(1);
    for (uint8_t n = 0; n < 16; n++) {
      auto *ptrack = empty_track.load_from_grid_512(n, 0);
      ptrack->init_track_type(A4_TRACK_TYPE);
      USE_LOCK();
      SET_LOCK();
      if (ptrack) ptrack->store_in_mem(0);
      CLEAR_LOCK();
    }
    uint16_t diff = clock_diff(cl, read_clock_ms());
    DEBUG_PRINT("Clock :");
    DEBUG_PRINTLN(diff);
  }
}


MCL mcl;
