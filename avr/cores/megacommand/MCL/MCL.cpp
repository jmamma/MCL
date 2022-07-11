#include "MCL_impl.h"
#include "ResourceManager.h"

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
  DEBUG_PRINTLN(BANK1_FILE_ENTRIES_END);
  bool ret = false;

  delay(100);

  ret = mcl_sd.sd_init();
  gfx.init_oled();
  R.Clear();
  R.use_icons_boot();

  if (BUTTON_DOWN(Buttons.BUTTON2)) {
    // gfx.draw_evil(R.icons_boot->evilknievel_bitmap);
    GUI.setPage(&boot_menu_page);
    while (GUI.currentPage() == &boot_menu_page) {
      GUI.loop();
    }
    return;
  }

  if (!ret) {
    oled_display.print("SD CARD ERROR :-(");
    oled_display.display();
    delay(2000);
    return;
  }

  gfx.splashscreen(R.icons_boot->mcl_logo_bitmap);

  ret = mcl_sd.load_init();

  GUI.addEventHandler((event_handler_t)&mcl_handleEvent);
  if (ret) {
    GUI.setPage(&grid_page);
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

  grid_task.init();

  GUI.addTask(&grid_task);
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

      case MDX_KEY_BANKA:
      case MDX_KEY_BANKB:
      case MDX_KEY_BANKC:
      case MDX_KEY_BANKD: {
        if (GUI.currentPage() == &grid_load_page ||
            GUI.currentPage() == &grid_save_page ||
            (GUI.currentPage() == &grid_page && grid_page.show_slot_menu)) {
          return false;
        }
        if (trig_interface.is_key_down(MDX_KEY_FUNC)) {
          return false;
        }
        if (grid_page.last_page == nullptr) {
          grid_page.last_page = GUI.currentPage();
        }
        GUI.setPage(&grid_page);
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
        if (GUI.currentPage() != &text_input_page &&
            GUI.currentPage() != &grid_save_page &&
            GUI.currentPage() != &grid_load_page &&
            !trig_interface.is_key_down(MDX_KEY_PATSONG)) {
          GUI.setPage(&page_select_page);
          return true;
        }
        return false;
      }
      case MDX_KEY_REC: {
        if (GUI.currentPage() != &seq_step_page &&
            GUI.currentPage() != &seq_ptc_page &&
            GUI.currentPage() != &seq_extstep_page) {
          seq_step_page.prepare = true;
          GUI.setPage(&seq_step_page);
        } else {
          if (seq_step_page.recording) {
            seq_step_page.recording = 0;
            MD.set_rec_mode(GUI.currentPage() == &seq_step_page);
            clearLed2();
            trig_interface.ignoreNextEvent(MDX_KEY_REC);
          } else {
            if (GUI.currentPage() == &seq_step_page) {
              GUI.setPage(&grid_page);
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
        if (GUI.currentPage() == &seq_step_page)
          break;
        if (GUI.currentPage() != &seq_ptc_page &&
            (trig_interface.is_key_down(MDX_KEY_SCALE) ||
             trig_interface.is_key_down(MDX_KEY_NO))) {
          // Ignore scale + copy if page != seq_step_page
          break;
        }
        opt_copy = 2;
        if (GUI.currentPage() == &seq_ptc_page ||
            GUI.currentPage() == &seq_extstep_page) {
          opt_copy = SeqPage::recording ? 2 : 1;
        }
        opt_copy_track_handler_cb();
        break;
      }
      case MDX_KEY_PASTE: {
        if (GUI.currentPage() == &seq_step_page)
          break;
        if (GUI.currentPage() != &seq_ptc_page &&
            (trig_interface.is_key_down(MDX_KEY_SCALE) ||
             trig_interface.is_key_down(MDX_KEY_NO))) {
          // Ignore scale + copy if page != seq_step_page
          break;
        }
        opt_paste = 2;
        if (GUI.currentPage() == &seq_ptc_page ||
            GUI.currentPage() == &seq_extstep_page) {
          opt_paste = SeqPage::recording ? 2 : 1;
        }
        reset_undo();
        opt_paste_track_handler();
        break;
      }
      case MDX_KEY_CLEAR: {
        if (GUI.currentPage() == &seq_step_page)
          break;
        if ((note_interface.notes_count_on() > 0) ||
            (trig_interface.is_key_down(MDX_KEY_SCALE) ||
             trig_interface.is_key_down(MDX_KEY_NO)))
          break;
        opt_clear = 2;
        //  MidiDevice *dev = midi_active_peering.get_device(UART2_PORT);
        if (GUI.currentPage() == &seq_ptc_page ||
            GUI.currentPage() == &seq_extstep_page) {
          //  if (SeqPage::midi_device == dev) {
          opt_clear = 1;
          //  } else {
          //    opt_clear = SeqPage::recording ? 2 : 1;
          //  }
        }
        opt_clear_track_handler();
        break;
      }
      case MDX_KEY_STOP: {
        grid_task.stop_hard_callback = true;
        break;
      }
      case MDX_KEY_FUNCEXTENDED: {
        MD.restore_kit_params();
        break;
      }
      }
    }

    if (event->mask == EVENT_BUTTON_RELEASED) {
      switch (key) {

      case MDX_KEY_REC: {
        if (!SeqPage::recording && (GUI.currentPage() == &seq_ptc_page ||
                                    GUI.currentPage() == &seq_extstep_page)) {
          if (GUI.currentPage() != &seq_step_page) {
            seq_step_page.prepare = true;
            GUI.setPage(&seq_step_page);
          }
          return true;
        }
      }
      case MDX_KEY_REALTIME: {
        return true;
      }
      }
    }
  }
}

MCL mcl;
