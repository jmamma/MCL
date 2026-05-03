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
#include "MCLStrings.h"


#include "GridPages.h"
#include "AuxPages.h"
#include "ProjectPages.h"
#include "SeqPages.h"
#include "SeqTrackUtil.h"

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
    { .ptr = &port1_menu_page },
    { .ptr = &port2_menu_page },
    { .ptr = &usbport_menu_page },
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

#ifdef PLATFORM_TBD
  // TBD has no dedicated BUTTON2 — boot menu opens if ENC1 or TOP_LEFT
  // (BUTTON1) is held at boot. Whichever one the user discovers, works.
  if (BUTTON_DOWN(Buttons.ENCODER1) || BUTTON_DOWN(Buttons.BUTTON1)) {
#else
  if (BUTTON_DOWN(Buttons.BUTTON2)) {
#endif
    // gfx.draw_evil(R.icons_boot->evilknievel_bitmap);
    mcl.setPage(BOOT_MENU_PAGE);
    while (mcl.currentPage() == BOOT_MENU_PAGE) {
      loop();
    }
  }
  if (!ret) {
    mcl_print_P(mclstr_sd_card_error);
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
  perf_page.encoder_check();
  key_interface.check_key_throttle();
  GUI.loop();
}

#ifdef PLATFORM_TBD
static bool tbd_rec_held = false;

bool tbd_handleEvent(gui_event_t *event) {
    // Track physical REC button state (unaffected by key_interface state resets)
    if (EVENT_BUTTON(event) && event->source == ButtonsClass::FUNC_BUTTON1) {
        tbd_rec_held = (event->mask == EVENT_BUTTON_PRESSED);
    }

    if (!EVENT_BUTTON(event)) {
        return false;
    }

    const bool sps_held   = key_interface.is_key_down(MDX_KEY_SPS);
    const bool is_press   = (event->mask == EVENT_BUTTON_PRESSED);
    const bool is_release = !(event->mask & 1);

    // ENC1 click toggles MCL PageSelect (tap-only). The release is honored
    // only when the press was a clean tap — not flagged as long-press by
    // the panel and the encoder didn't rotate while held. armed=false
    // rejects the spurious boot-time release before any press.
    if (event->source == ButtonsClass::ENCODER1) {
        static bool enc1_armed = false;
        if (is_press) {
            enc1_armed = true;
            return true;
        }
        if (enc1_armed && !Buttons.enc1_long_press_seen
                       && !Buttons.enc1_rotated_while_held) {
            if (mcl.currentPage() == PAGE_SELECT_PAGE) {
                page_select_page.close_to_selection();
            } else {
                mcl.setPage(PAGE_SELECT_PAGE);
            }
        }
        enc1_armed = false;
        return true;
    }

    // ENC4 tap latches/unlatches the per-page shift menu (seq menu on
    // sequencer pages, slot menu on the grid page). pollTBD mirrors the
    // latch onto BUTTON3.B_CURRENT, so each toggle synthesizes a normal
    // BUTTON3 press or release pair — the existing menu open/apply
    // handlers in SeqPage and GridPage consume them unchanged. The MDX
    // passthrough modifier lives on MCL_B (MDX_KEY_SPS), not here.
    // Same tap gating as ENC1.
    if (event->source == ButtonsClass::ENCODER4) {
        static bool enc4_armed = false;
        if (is_press) {
            enc4_armed = true;
            return true;
        }
        if (enc4_armed && !Buttons.enc4_long_press_seen
                       && !Buttons.enc4_rotated_while_held) {
            Buttons.tbd_menu_latched = !Buttons.tbd_menu_latched;
        }
        enc4_armed = false;
        return true;
    }

    // SPS + TOP_RIGHT (BUTTON4) -> BANK_GROUP. Suppress BUTTON4 to MCL pages.
    if (sps_held && event->source == ButtonsClass::BUTTON4) {
        key_interface.key_event(MDX_KEY_BANKGROUP, is_release);
        return true;
    }

    // BUTTON1..BUTTON4 are MCL local roles handled by mcl_handleEvent.
    // GridPage's native BUTTON1+BUTTON4 chord (SYSTEM_PAGE) is preserved here.
    if (event->source <= ButtonsClass::BUTTON4) {
        return false;
    }

    uint8_t key = 255;

    // SPS-passthrough: while MCL_B is held, panel keys reroute to MDX keys.
    // Arrows on grid page open the bank overlay (eaten locally — they no
    // longer fire MD bank-key events). Transport remaps to menus. Suppresses
    // the source's normal role.
    if (sps_held) {
        const bool is_arrow = (event->source == ButtonsClass::FUNC_BUTTON6 ||
                               event->source == ButtonsClass::FUNC_BUTTON7 ||
                               event->source == ButtonsClass::FUNC_BUTTON8 ||
                               event->source == ButtonsClass::FUNC_BUTTON9);
        if (is_arrow && mcl.currentPage() == GRID_PAGE &&
            grid_page.bank_popup_external) {
            if (is_press) {
                grid_page.enter_bank_overlay();
            } else {
                // Only exit the overlay when no other arrow is still held.
                // The releasing arrow is still B_CURRENT==down for one tick;
                // skip its own slot so we don't lock the overlay open.
                bool any_other_arrow =
                    (event->source != ButtonsClass::FUNC_BUTTON6 &&
                     BUTTON_DOWN(ButtonsClass::FUNC_BUTTON6)) ||
                    (event->source != ButtonsClass::FUNC_BUTTON7 &&
                     BUTTON_DOWN(ButtonsClass::FUNC_BUTTON7)) ||
                    (event->source != ButtonsClass::FUNC_BUTTON8 &&
                     BUTTON_DOWN(ButtonsClass::FUNC_BUTTON8)) ||
                    (event->source != ButtonsClass::FUNC_BUTTON9 &&
                     BUTTON_DOWN(ButtonsClass::FUNC_BUTTON9));
                if (!any_other_arrow) {
                    grid_page.exit_bank_overlay();
                }
            }
            return true;
        }
        switch (event->source) {
            case ButtonsClass::FUNC_BUTTON1: key = MDX_KEY_KIT;      break; // SPS + REC
            case ButtonsClass::FUNC_BUTTON2: key = MDX_KEY_GLOBAL;   break; // SPS + PLAY
            case ButtonsClass::FUNC_BUTTON3: key = MDX_KEY_SCALE;    break; // SPS + STOP
            case ButtonsClass::FUNC_BUTTON5: key = MDX_KEY_EXTENDED; break; // SPS + NO
            default: break;
        }
        if (key != 255) {
            key_interface.key_event(key, is_release);
            return true;
        }
    }

    // Trig buttons. Restricted to TRIG_BUTTON1..TRIG_BUTTON16 — TBD_KEY_* IDs
    // sit immediately above this range and must reach the else-branch switch.
    if (event->source >= ButtonsClass::TRIG_BUTTON1 &&
        event->source <  ButtonsClass::TRIG_BUTTON1 + 16) {
        key = event->source - ButtonsClass::TRIG_BUTTON1; // MDX_KEY_TRIG1
        // Bank overlay (arrow held during external pattern-select) owns
        // trig events — eat them locally so they don't pollute
        // note_interface or fall through to the pattern-load path.
        if (mcl.currentPage() == GRID_PAGE && grid_page.bank_overlay_active) {
            if (is_press) {
                int8_t b = -1;
                if (key < 4)                    b = key;
                else if (key >= 8 && key < 12)  b = key - 8 + 4;
                if (b >= 0) {
                    grid_page.bank = b;
                    grid_page.bank_pick_trig = key;
                    grid_page.bank_popup_pending_trig = 0xFF;
                    // Repaint overlay so the new current bank blinks.
                    grid_page.enter_bank_overlay();
                }
            } else if (is_release && grid_page.bank_pick_trig == key) {
                grid_page.bank_pick_trig = 0xFF;
            }
            return true; // eat trig events while overlay is active
        }
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
                    if (tbd_rec_held) {
                      // REC + PLAY: enable sequencer record mode, start clock if needed
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
            case ButtonsClass::FUNC_BUTTON5:  key = MDX_KEY_NO;    break;
            case ButtonsClass::FUNC_BUTTON6:  key = MDX_KEY_UP;    break;
            case ButtonsClass::FUNC_BUTTON7:  key = MDX_KEY_LEFT;  break;
            case ButtonsClass::FUNC_BUTTON8:  key = MDX_KEY_DOWN;  break;
            case ButtonsClass::FUNC_BUTTON9:  key = MDX_KEY_RIGHT; break;
            case ButtonsClass::TBD_KEY_FUNC:  key = MDX_KEY_FUNC;  break;
            case ButtonsClass::TBD_KEY_YES:   key = MDX_KEY_YES;   break;
            case ButtonsClass::TBD_KEY_PAGE:  key = MDX_KEY_PAGE;  break;
            case ButtonsClass::TBD_KEY_SPS:   key = MDX_KEY_SPS;   break;
            default: break;
        }
    }

    if (key != 255) {
        const bool is_arrow = (key == MDX_KEY_UP || key == MDX_KEY_DOWN ||
                               key == MDX_KEY_LEFT || key == MDX_KEY_RIGHT);
        // Arrows on the grid page navigate MCL, not the MD's GUI.
        const bool md_forward = MD.connected &&
            !(is_arrow && mcl.currentPage() == GRID_PAGE);
        if (md_forward && (is_arrow ||
                           key == MDX_KEY_YES || key == MDX_KEY_NO)) {
            if (!is_release && key_interface.is_key_down(key)) {
                return true; // suppress key repeat
            }
            if (is_release) {
                switch (key) {
                    case MDX_KEY_UP:    CLEAR_BIT64(key_interface.cmd_key_state, key); MD.release_up_arrow();    break;
                    case MDX_KEY_DOWN:  CLEAR_BIT64(key_interface.cmd_key_state, key); MD.release_down_arrow();  break;
                    case MDX_KEY_LEFT:  CLEAR_BIT64(key_interface.cmd_key_state, key); MD.release_left_arrow();  break;
                    case MDX_KEY_RIGHT: CLEAR_BIT64(key_interface.cmd_key_state, key); MD.release_right_arrow(); break;
                    case MDX_KEY_YES:   MD.release_yes_button(); break;
                    case MDX_KEY_NO:    MD.release_no_button();  break;
                    default: break;
                }
            } else {
                switch (key) {
                    case MDX_KEY_UP:    SET_BIT64(key_interface.cmd_key_state, key); MD.hold_up_arrow();    break;
                    case MDX_KEY_DOWN:  SET_BIT64(key_interface.cmd_key_state, key); MD.hold_down_arrow();  break;
                    case MDX_KEY_LEFT:  SET_BIT64(key_interface.cmd_key_state, key); MD.hold_left_arrow();  break;
                    case MDX_KEY_RIGHT: SET_BIT64(key_interface.cmd_key_state, key); MD.hold_right_arrow(); break;
                    case MDX_KEY_YES:   MD.press_yes_button();   break;
                    case MDX_KEY_NO:    MD.press_no_button();    break;
                }
            }
            return true;
        }
        key_interface.key_event(key, is_release);

        // MCL_B held on grid page enters single-action pattern select for
        // the current bank. While held, an arrow modifier (handled above)
        // flips into the bank overlay so a different bank can be picked.
        // Release closes the popup.
        if (key == MDX_KEY_SPS && mcl.currentPage() == GRID_PAGE) {
            if (is_press) {
                grid_page.open_pattern_select();
            } else {
                grid_page.close_bank_popup();
            }
        }
        return true;
    }
    return false;
}
#endif // PLATFORM_TBD

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
        // MCL_B bank-select stage owns the popup — ignore MD bank keys until
        // the modifier is released.
        if (grid_page.bank_popup_external) {
          return true;
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
            GUI_hardware.led.rec_active = false;
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
#if !defined(__AVR__)
        // SPSX: REALTIME held = fill mode for SPSX_COND_FILL trigs.
        // Initial press still bootstraps record; release clears fill.
        if (mcl_seq.using_spsx_tracks) {
          mcl_seq.set_fill(true);
        }
#endif
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
        //  MidiDevice *dev = midi_active_peering.dev2;
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
#if !defined(__AVR__)
        if (mcl_seq.using_spsx_tracks) {
          mcl_seq.set_fill(false);
        }
#endif
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
    uint16_t diff = clock_diff(cl, read_clock_ms());
    DEBUG_PRINT("Clock :");
    DEBUG_PRINTLN(diff);
  }
}


MCL mcl;
