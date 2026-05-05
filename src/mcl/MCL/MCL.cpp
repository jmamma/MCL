#include "mcl.h"
#include "ResourceManager.h"
#include "MCLSd.h"
#include "MCLGFX.h"
#include "MCLGUI.h"
#include "GridTrack.h"
#include "GridTask.h"
#include "EmptyTrack.h"
#include "MDTrackSelect.h"
#include "../Drivers/MD/MD.h"
#include "../Drivers/MNM/MNM.h"
#include "../Drivers/A4/A4.h"
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
#ifdef PLATFORM_TBD
#include "BankPopupPage.h"
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

  if (BUTTON_DOWN(Buttons.BUTTON2)) {
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

  load_persistent_resources();

  // tbd_handleEvent runs from GuiClass::handleTopEvent (under PLATFORM_TBD)
  // so it can preempt the active page on cluster overrides.
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

#ifdef PLATFORM_TBD
  midi_active_peering.dev1->ui_loop();
  midi_active_peering.dev2->ui_loop();
#endif

  key_interface.check_key_throttle();
  GUI.loop();
}

#ifdef PLATFORM_TBD
// Tap window shared with all TBD-panel tap gestures — see TBD_TAP_MAX_MS
// in SpsMode.h. Anything held longer is treated as a hold.

static bool tbd_rec_held = false;

// Push the BankPopupPage; its setup()/init() handle state seeding and
// MD-side bank-group sync. Returns true if the page was pushed.
static bool tbd_open_bank_popup() {
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

// While the bank popup is up, arrows navigate the 8-bank grid:
//   LEFT/RIGHT scroll bank by ±1 across all 8 banks (wraps A↔H).
//   UP/DOWN flip group (bank ^= 4) — a quick same-letter jump.
static bool tbd_handle_bank_arrow_cycle(gui_event_t *event) {
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

  // Any arrow press while the popup is up brings the OLED grid back —
  // useful after pattern selection has hidden it.
  grid_page.bank_popup_oled_visible = true;

  // Source of truth is grid_page.bank (0..7). LEFT/RIGHT walk linearly
  // across all 8 banks (wrapping); UP/DOWN flip the group bit. Either
  // path may cross the group boundary, so we re-sync MD.currentBank
  // whenever the new bank lands in a different group.
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

bool tbd_handleEvent(gui_event_t *event) {
    // Track physical REC button state (unaffected by key_interface state resets)
    if (EVENT_BUTTON(event) && event->source == ButtonsClass::FUNC_BUTTON1) {
        tbd_rec_held = (event->mask == EVENT_BUTTON_PRESSED);
    }

    if (!EVENT_BUTTON(event)) {
        return false;
    }

    const bool is_press   = (event->mask == EVENT_BUTTON_PRESSED);
    const bool is_release = !(event->mask & 1);

    // Menu/config page remap: while a MenuPage is active (system, midi
    // config, mcl config, MIDI port menus, etc.), TL replaces BUTTON1
    // (NO/exit) and TR replaces BUTTON4 (YES/enter) so the user can
    // navigate the menu without leaving the cluster. Stash the original
    // source first so the A / X cluster handlers below can distinguish
    // physical A / X events from the remapped TL / TR ones (and only
    // emit MDX_KEY_NO / MDX_KEY_YES for the physical sources).
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

    // TL → TR chord opens the system config page. Asymmetric on purpose:
    // only fires when TBD_BUTTON_TR is the press edge while
    // BUTTON2 (TL) is already held, so the reverse order falls through
    // to the normal SPS toggle. Suppresses the SPS-mode latch for this
    // TR press by short-circuiting before handle_toggle_button.
    if (event->source == ButtonsClass::TBD_BUTTON_TR && is_press &&
        BUTTON_DOWN(ButtonsClass::BUTTON2)) {
      mcl.pushPage(SYSTEM_PAGE);
      midi_active_peering.dev1->mark_tr_consumed();
      midi_active_peering.dev2->mark_tr_consumed();
      return true;
    }

    if (midi_active_peering.dev1->handle_ui_event(event)) return true;
    if (midi_active_peering.dev2->handle_ui_event(event)) return true;

    // BUTTON4 (cluster X). Emits MDX_KEY_YES + MD YES sysex only in
    // SPS-latched mode (MDX_KEY_YES passthrough for the MD's UI).
    // Only physical X (orig_src == BUTTON4) emits — a remapped TR on
    // a menu page falls through so MenuPage's BUTTON4 enter runs.
    // The event is fully consumed in SPS mode so GridPage's BUTTON4
    // release handler (GridPage.cpp:1223 → GRID_LOAD_PAGE) can't fire.
    static bool x_just_toggled = false;
    if (event->source == ButtonsClass::BUTTON4 &&
        orig_src == ButtonsClass::BUTTON4) {
      const PageIndex pg = mcl.currentPage();
      if (is_press && pg == GRID_PAGE) {
        mcl.setPage(GRID_LOAD_PAGE);
        x_just_toggled = true;
        return true;
      }
      if (is_press && pg == GRID_LOAD_PAGE) {
        mcl.setPage(GRID_PAGE);
        x_just_toggled = true;
        return true;
      }
      if (is_release && x_just_toggled) {
        x_just_toggled = false;
        return true;
      }
    }

    // BUTTON1 (cluster Y) routing.
    //   GRID_PAGE: Y press opens GRID_SAVE_PAGE; Y release on entry is
    //              dropped via ignoreNextEvent so we don't commit on tail.
    //   GRID_SAVE_PAGE / GRID_LOAD_PAGE: Y press closes back to GRID_PAGE
    //              (toggle behaviour). Release of the closing press is
    //              also dropped so GridPage doesn't see a stray BUTTON1
    //              release that opens save again. Group save/load on TBD
    //              is driven by A (MDX_KEY_NO), not Y.
    //   Anywhere else: fall through.
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

    // ENC1 tap toggles the bank popup (pattern-select gesture). "Tap" =
    // pressed + released < TBD_TAP_MAX_MS with no rotation in the
    // window; anything longer is a rotation grip even if pollTBD didn't
    // latch enc1_rotated_while_held in time. Replaces the AVR's BANK
    // key trigger, which TBD has no panel button for.
    if (event->source == ButtonsClass::ENCODER1) {
        // PageSelectPage owns the encoder cluster (ENC1..4 press =
        // category selector). Skip the bank-popup tap handler entirely
        // so PageSelectPage::handleEvent picks up the press.
        if (mcl.currentPage() == PAGE_SELECT_PAGE) return false;
        static bool enc1_armed = false;
        static uint16_t enc1_press_ms = 0;
        static constexpr uint16_t kEnc1TapMaxMs = TBD_TAP_MAX_MS;
        if (is_press) {
            enc1_armed = true;
            enc1_press_ms = read_clock_ms();
            return true;
        }
        const bool too_long =
            clock_diff(enc1_press_ms, read_clock_ms()) > kEnc1TapMaxMs;
        if (enc1_armed && !Buttons.enc1_long_press_seen
                       && !Buttons.enc1_rotated_while_held
                       && !too_long) {
            if (mcl.currentPage() == BANK_POPUP_PAGE) {
                bank_popup_page.close();
            } else {
                tbd_open_bank_popup();
            }
        }
        enc1_armed = false;
        return true;
    }

    // SPS-latched encoder taps (ENC2/3/4). Same 150 ms tap window and
    // long-press / rotated-while-held gating as ENC1. Outside SPS mode
    // these clicks are consumed but no-op (pages don't see them).
    //   ENC2: TEMPO  — bare toggle_tempo_window; FUNC tap_tempo
    //   ENC3: PATSONG — bare press_patternsong_button; FUNC=GLOBAL chord
    //         (hold_function_button → toggle_global_window → release)
    //   ENC4: KIT     — bare toggle_kit_menu; FUNC=KIT chord
    //         (hold_function_button → toggle_kit_menu → release)
    // ENC2..4 taps in normal mode trigger local MCL actions (Grid swap/Seq advance).
    // Handled by driver handle_ui_event in SPS mode.
    if (event->source >= ButtonsClass::ENCODER2 &&
        event->source <= ButtonsClass::ENCODER4) {
        // Fall through to mcl_handleEvent if the driver didn't consume it.
    }

    // BUTTON1..BUTTON4 are MCL local roles handled by mcl_handleEvent.
    // GridPage's native BUTTON1+BUTTON4 chord (SYSTEM_PAGE) is preserved here.
    if (event->source <= ButtonsClass::BUTTON4) {
        return false;
    }

    uint8_t key = 255;

    // Bank popup arrow navigation — works in either mode whenever the
    // popup is up (the popup itself is opened via ENC1 tap).
    if (tbd_handle_bank_arrow_cycle(event)) return true;

    // Trig buttons. Restricted to TRIG_BUTTON1..TRIG_BUTTON16 — TBD_BUTTON_* IDs
    // sit immediately above this range and must reach the else-branch switch.
    if (event->source >= ButtonsClass::TRIG_BUTTON1 &&
        event->source <  ButtonsClass::TRIG_BUTTON1 + 16) {
        key = event->source - ButtonsClass::TRIG_BUTTON1; // MDX_KEY_TRIG1

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
            // TBD NO transport button is the universal MDX_KEY_FUNC source
            // (both modes). In normal mode this enables FUNC chords (FUNC +
            // arrow → MD windows, FUNC + REC/PLAY/STOP → COPY/CLEAR/PASTE,
            // etc.); in SPS-latched mode the cluster Y/X/A fire menu-open
            // sysex directly so FUNC isn't strictly needed there but stays
            // consistent.
            case ButtonsClass::FUNC_BUTTON5:  key = MDX_KEY_FUNC;  break;
            case ButtonsClass::FUNC_BUTTON6:  key = MDX_KEY_UP;    break;
            case ButtonsClass::FUNC_BUTTON7:  key = MDX_KEY_LEFT;  break;
            case ButtonsClass::FUNC_BUTTON8:  key = MDX_KEY_DOWN;  break;
            case ButtonsClass::FUNC_BUTTON9:  key = MDX_KEY_RIGHT; break;
            // MCL_B is a pure modifier (BUTTON_DOWN-checked):
            //   normal mode + trig    = track select
            //   SPS-latched + trig    = sub-page selector
            //   either + arrow        = sub-page traversal
            // MDX_KEY_FUNC is provided by TBD NO (FUNC_BUTTON5 above), so B
            // doesn't need to emit anything.
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


void MCL::load_persistent_resources() {
#if !defined(__AVR__)
  R.Clear();
  R.use_machine_param_names();
  R.use_machine_names_short();
  R.SetPersistent();
#endif
}

MCL mcl;
