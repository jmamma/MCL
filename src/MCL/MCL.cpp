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

const lightpage_ptr_t MCL::pages_table[NUM_PAGES] PROGMEM = { 
    { .ptr = &grid_page },           // Index: 0
    { .ptr = &page_select_page },    // Index: 1
    { .ptr = &system_page },         // Index: 2
    { .ptr = &mixer_page },          // Index: 3
    { .ptr = &grid_save_page },      // Index: 4
    { .ptr = &grid_load_page },      // Index: 5
#ifdef WAV_DESIGNER
    { .ptr = &wd.mixer },            // Index: 6
#endif
    { .ptr = &seq_step_page },       // Index: 7
    { .ptr = &seq_extstep_page },    // Index: 8
    { .ptr = &seq_ptc_page },        // Index: 9
    { .ptr = &text_input_page },     // Index: 10
    { .ptr = &poly_page },           // Index: 11
    { .ptr = &sample_browser },      // Index: 12
    { .ptr = &questiondialog_page }, // Index: 13
    { .ptr = &start_menu_page },     // Index: 14
    { .ptr = &boot_menu_page },      // Index: 15
    { .ptr = &fx_page_a },           // Index: 16
    { .ptr = &fx_page_b },           // Index: 17
#ifdef WAV_DESIGNER
    { .ptr = &wd.pages[0] },         // Index: 18
    { .ptr = &wd.pages[1] },         // Index: 19
    { .ptr = &wd.pages[2] },         // Index: 20
#endif
    { .ptr = &route_page },          // Index: 21
    { .ptr = &lfo_page },            // Index: 22
    { .ptr = &ram_page_a },          // Index: 23
    { .ptr = &ram_page_b },          // Index: 24
    { .ptr = &load_proj_page },      // Index: 25
    { .ptr = &midi_config_page },    // Index: 26
    { .ptr = &md_config_page },      // Index: 27
    { .ptr = &chain_config_page },   // Index: 28
    { .ptr = &aux_config_page },     // Index: 29
    { .ptr = &mcl_config_page },     // Index: 30
    { .ptr = &arp_page },            // Index: 31
    { .ptr = &md_import_page },      // Index: 32
    { .ptr = &midiport_menu_page },  // Index: 33
    { .ptr = &midiprogram_menu_page }, // Index: 34
    { .ptr = &midiclock_menu_page }, // Index: 35
    { .ptr = &midiroute_menu_page }, // Index: 36
    { .ptr = &midimachinedrum_menu_page }, // Index: 37
    { .ptr = &midigeneric_menu_page }, // Index: 38
    { .ptr = &sound_browser },       // Index: 39
    { .ptr = &perf_page }           // Index: 40
};

void mcl_setup() { mcl.setup(); }

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

  DEBUG_PRINTLN("bank1 end: ");
  DEBUG_PRINTLN(BANK3_FILE_ENTRIES_END);
  bool ret = false;

  delay(50);
#ifdef CHECKSUM
  bool health = health_check();
#endif
  ret = mcl_sd.sd_init();
  gfx.init_oled();

#ifdef CHECKSUM
  if (!health) {
    oled_display.textbox("CHECKSUM ", "ERROR");
    oled_display.display();
    while (1);
  }
#endif

  R.Clear();
  R.use_icons_boot();

  if (BUTTON_DOWN(Buttons.BUTTON2)) {
    // gfx.draw_evil(R.icons_boot->evilknievel_bitmap);
    mcl.setPage(BOOT_MENU_PAGE);
    while (mcl.currentPage() == BOOT_MENU_PAGE) {
      GUI.loop();
    }
    return;
  }

  if (!ret) {
    oled_display.print(F("SD CARD ERROR :-("));
    oled_display.display();
    delay(2000);
    return;
  }

  gfx.splashscreen(R.icons_boot->mcl_logo_bitmap);

  ret = mcl_sd.load_init();

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

  trig_interface.setup(&Midi);
  trig_interface.enable_listener();

  md_track_select.setup(&Midi);

#ifdef EXT_TRACKS
  A4SysexListener.setup(&Midi2);
  MNMSysexListener.setup(&Midi2);
#endif
  perf_page.setup();

  grid_task.init();

  GUI.addTask(&grid_task);
  g_clock_ms = 0;
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

bool mcl_handleEvent(gui_event_t *event) {
  /*
  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    MidiDevice *device = midi_active_peering.get_device(port);

    uint8_t track = event->source - 128;
    if (device != &MD) {
      return true;
    }
  }
  */
  if (EVENT_CMD(event)) {
    uint8_t key = event->source - 64;
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
                trig_interface_task.setup();
                GUI.addTask(&trig_interface_task);
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
//            (mcl.currentPage() == MIXER_PAGE && mixer_page.preview_mute_set != 255)) {
          return false;
        }
        if (trig_interface.is_key_down(MDX_KEY_FUNC)) {
          return false;
        }
        if (grid_page.last_page == 255) {
          grid_page.last_page = mcl.currentPage();
        }
        mcl.setPage(GRID_PAGE);
        grid_page.bank_popup = 1;
        grid_page.bank_popup_loadmask = 0;
        bool clear_states = false;
        trig_interface.on(clear_states);
        grid_page.bank = key - MDX_KEY_BANKA + MD.currentBank * 4;
        uint16_t *mask = (uint16_t *)&grid_page.row_states[0];
        MD.set_trigleds(mask[grid_page.bank], TRIGLED_EXCLUSIVENDYNAMIC);

        grid_page.send_row_led();

        // uint8_t row = grid_page.bank * 16;
        // grid_page.jump_to_row(row);
        return true;
      }
      case MDX_KEY_BANKGROUP: {
        if (mcl.currentPage() != TEXT_INPUT_PAGE &&
            mcl.currentPage() != GRID_SAVE_PAGE &&
            mcl.currentPage() != GRID_LOAD_PAGE &&
            !trig_interface.is_key_down(MDX_KEY_PATSONG)) {
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
            trig_interface.ignoreNextEvent(MDX_KEY_REC);
          } else {
            if (mcl.currentPage() == SEQ_STEP_PAGE) {
              trig_interface.ignoreNextEvent(MDX_KEY_REC);
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
            (trig_interface.is_key_down(MDX_KEY_SCALE) ||
             trig_interface.is_key_down(MDX_KEY_NO))) {
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
            (trig_interface.is_key_down(MDX_KEY_SCALE) ||
             trig_interface.is_key_down(MDX_KEY_NO))) {
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
            (trig_interface.is_key_down(MDX_KEY_SCALE) ||
             trig_interface.is_key_down(MDX_KEY_NO)))
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
        trig_interface.ignoreNextEvent(MDX_KEY_EXTENDED);
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
        trig_interface.ignoreNextEventClear(MDX_KEY_EXTENDED);
        return true;
      }
      }
    }
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    mcl.setPage(PAGE_SELECT_PAGE);
    return true;
  }

  return false;
}

/*
uint32_t read_bytes_progmem(uint32_t address, uint8_t n) {
    uint32_t value = 0;
    for (uint8_t i = 0; i < n; i++) {
        uint8_t byte = pgm_read_byte_far(address + i); // Read a byte from program memory
        value |= (uint32_t)byte << (8 * i); // Combine bytes into a 32-bit value
    }
    return value;
}


bool health_check() {
   uint32_t memoryAddress = 256 * 1024 - 16 * 1024 - 6;
   uint32_t length = read_bytes_progmem(memoryAddress, 4);
   uint16_t checksum = (uint16_t) read_bytes_progmem(memoryAddress + 4, 2);
   DEBUG_PRINTLN(length);
   DEBUG_PRINTLN(checksum);

   uint16_t calc_checksum = 0;
   uint32_t n = 0;
   uint8_t byte = 0;

   for (uint32_t n = 0; n < length; n++) {
     uint8_t last_byte = byte;
     byte = pgm_read_byte_far(n);
     if (n % 2 == 0) {
       calc_checksum ^= (byte << 8) | last_byte;
     }
   }

   DEBUG_PRINTLN(calc_checksum);

#ifdef DEBUGMODE
   if (calc_checksum != checksum) {
      DEBUG_PRINTLN("checksum error");
   }
#endif

   return calc_checksum == checksum;

}
*/

void sdcard_bench() {

  EmptyTrack empty_track;
  DeviceTrack *ptrack;
  while (1) {
    uint16_t cl = g_clock_ms;
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
    uint16_t diff = clock_diff(cl, g_clock_ms);
    DEBUG_PRINT("Clock :");
    DEBUG_PRINTLN(diff);
  }
}


MCL mcl;
