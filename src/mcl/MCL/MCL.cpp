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
#include "SpsMode.h"

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
  sps_mode.poll_encoders();
  key_interface.check_key_throttle();
  GUI.loop();
}

#ifdef PLATFORM_TBD
// Max press-to-release window (ms) that still counts as an encoder tap
// for the SPS-mode encoder cluster gestures. Above this, the press is
// treated as a hold and any tap-action (BANKGROUP, TEMPO, etc.) is
// suppressed.
#define TBD_ENC_TAP_MAX_MS 175

static bool tbd_rec_held = false;

// Open the grid bank popup at the current bank, in countdown state. Used
// by the TBD NO button outside SPS-mode to trigger a bank-select gesture
// without holding a hardware BANK key. Returns true if the popup opened.
static bool tbd_open_bank_popup() {
  PageIndex pg = mcl.currentPage();
  if (pg == GRID_LOAD_PAGE || pg == GRID_SAVE_PAGE ||
      pg == TEXT_INPUT_PAGE) {
    return false;
  }
  if (pg == GRID_PAGE && grid_page.show_slot_menu) return false;

  if (grid_page.last_page == 255 && pg != GRID_PAGE) {
    grid_page.last_page = pg;
  }
  if (pg != GRID_PAGE) {
    mcl.setPage(GRID_PAGE);
  }
  // State 2 = "popup up". We skip state 1 since there's no hardware BANK
  // key being held. The auto-close countdown is disabled on TBD (see
  // GridPage::loop) — the popup stays up until trig→pattern picks one or
  // a re-press of NO closes it.
  grid_page.bank_popup = 2;
  grid_page.bank_popup_loadmask = 0;
  grid_page.bank_popup_oled_visible = true;
  bool clear_states = false;
  key_interface.on(clear_states);
  // Sync MD.currentBank to whichever group grid_page.bank lives in.
  // Without this the arrow-cycle math underflows: if MCL has bank A
  // (=0) selected but the MD is in group 1, letter = 0 - 4 wraps to
  // 252, and RIGHT arrow lands on F instead of B.
  uint8_t group = grid_page.bank / 4;
  if (MD.currentBank != group) {
    MD.currentBank = group;
    if (MD.connected) MD.press_bankgroup_button();
  }
  uint16_t *mask = (uint16_t *)&grid_page.row_states[0];
  mcl_gui.set_trigleds(mask[grid_page.bank], TRIGLED_EXCLUSIVENDYNAMIC);
  grid_page.send_row_led();
  if (MD.connected) MD.draw_bank(grid_page.bank % 4);
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

    if (sps_mode.handle_toggle_button(event)) return true;

    // ENC1 tap sends MD BANKGROUP (MD_GUI_BANKGROUP / 0x40 sysex) so the
    // MD's own bank-group toggle is reachable from the TBD without giving
    // up the encoder for rotation. "Tap" = pressed + released < 150 ms
    // with no rotation in the window; anything longer is a rotation grip
    // even if pollTBD didn't latch enc1_rotated_while_held in time.
    if (event->source == ButtonsClass::ENCODER1) {
        static bool enc1_armed = false;
        static uint16_t enc1_press_ms = 0;
        static constexpr uint16_t kEnc1TapMaxMs = TBD_ENC_TAP_MAX_MS;
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
            if (MD.connected) MD.press_bankgroup_button();
        }
        enc1_armed = false;
        return true;
    }

    // SPS-latched encoder taps (ENC2/3/4). Same 150 ms tap window and
    // long-press / rotated-while-held gating as ENC1. Outside SPS mode
    // these clicks are consumed but no-op (pages don't see them).
    //   ENC2: TEMPO  — bare tap_tempo;       FUNC toggle_tempo_window
    //   ENC3: PATSONG — bare press_patternsong_button; FUNC=GLOBAL chord
    //         (hold_function_button → toggle_global_window → release)
    //   ENC4: KIT     — bare toggle_kit_menu; FUNC=KIT chord
    //         (hold_function_button → toggle_kit_menu → release)
    if (event->source >= ButtonsClass::ENCODER2 &&
        event->source <= ButtonsClass::ENCODER4) {
        static bool   enc_armed[3]    = {false, false, false};
        static uint16_t enc_press_ms[3] = {0, 0, 0};
        static constexpr uint16_t kEncTapMaxMs = TBD_ENC_TAP_MAX_MS;
        const uint8_t idx = event->source - ButtonsClass::ENCODER2;
        const bool *long_seen[3] = {&Buttons.enc2_long_press_seen,
                                    &Buttons.enc3_long_press_seen,
                                    &Buttons.enc4_long_press_seen};
        const bool *rot_seen[3]  = {&Buttons.enc2_rotated_while_held,
                                    &Buttons.enc3_rotated_while_held,
                                    &Buttons.enc4_rotated_while_held};
        if (is_press) {
            enc_armed[idx]    = true;
            enc_press_ms[idx] = read_clock_ms();
            return true;
        }
        const bool too_long =
            clock_diff(enc_press_ms[idx], read_clock_ms()) > kEncTapMaxMs;
        const bool tap_valid = enc_armed[idx] && !*long_seen[idx]
                            && !*rot_seen[idx] && !too_long;
        enc_armed[idx] = false;
        if (!tap_valid)              return true;
        if (!sps_mode.is_active())   return true;
        if (!MD.connected)           return true;
        const bool func_held = key_interface.is_key_down(MDX_KEY_FUNC);
        switch (event->source) {
            case ButtonsClass::ENCODER2: // TEMPO
                if (func_held) MD.toggle_tempo_window();
                else           MD.tap_tempo();
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

    // SPS-mode cluster override: while latched, Y/X/A bypass their local
    // NO/YES/shift actions and fire MD menu-open sysex directly.
    if (sps_mode.handle_cluster_menus(event)) return true;

    // BUTTON1..BUTTON4 are MCL local roles handled by mcl_handleEvent.
    // GridPage's native BUTTON1+BUTTON4 chord (SYSTEM_PAGE) is preserved here.
    if (event->source <= ButtonsClass::BUTTON4) {
        return false;
    }

    uint8_t key = 255;

    // SPS sub-page traversal takes precedence over the FUNC+arrow chord —
    // both gestures share the SPS-key-held trigger but the latch makes
    // sub-page navigation the intent. Subpage also flags the B-tap
    // handler so the trailing B release doesn't double-fire LFO/PAGE.
    if (sps_mode.handle_arrow_subpage(event))    return true;
    if (sps_mode.handle_func_arrow_chord(event)) return true;
    // MCL_B press/release: arms / fires LFO/PAGE on tap-only release in
    // SPS-latched mode. Must run after handle_arrow_subpage so chord
    // detection has set b_chorded_ before B's release lands.
    if (sps_mode.handle_sps_key_tap(event))      return true;

    // Bank popup arrow navigation — works in either mode whenever the
    // popup is up (the popup itself is opened via ENC1 tap).
    if (tbd_handle_bank_arrow_cycle(event)) return true;

    // Trig buttons. Restricted to TRIG_BUTTON1..TRIG_BUTTON16 — TBD_KEY_* IDs
    // sit immediately above this range and must reach the else-branch switch.
    if (event->source >= ButtonsClass::TRIG_BUTTON1 &&
        event->source <  ButtonsClass::TRIG_BUTTON1 + 16) {
        key = event->source - ButtonsClass::TRIG_BUTTON1; // MDX_KEY_TRIG1

        if (sps_mode.handle_trig_forward(event, key)) return true;

        // FUNC held + trig in normal (non-latched) mode → MD track
        // select. FUNC is sourced from TBD NO (FUNC_BUTTON5) which sets
        // MDX_KEY_FUNC on its press.
        if (!sps_mode.is_active() &&
            key_interface.is_key_down(MDX_KEY_FUNC)) {
            if (is_press && key < NUM_MD_TRACKS) {
                MD.currentTrack = key;
                if (MD.connected) MD.track_select(key + 1);
            }
            return true;
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
            case ButtonsClass::TBD_KEY_SPS: break;
            default: break;
        }
    }

    if (key != 255) {
        const bool is_arrow = (key == MDX_KEY_UP || key == MDX_KEY_DOWN ||
                               key == MDX_KEY_LEFT || key == MDX_KEY_RIGHT);

        // Arrows: forward to MD's GUI only on pages that don't consume
        // them locally. Grid + sequencer pages use arrows for their own
        // navigation (note pitch on SEQ_PTC_PAGE, step / track on the
        // step-edit pages, row/col on GRID_PAGE), so route those through
        // the local key_event path instead. SPS-latched mode always
        // forwards — the cluster owns MD navigation while latched, so
        // arrows belong to the MD regardless of the active page.
        const PageIndex cur_pg = mcl.currentPage();
        const bool arrows_local =
            !sps_mode.is_active() &&
            (cur_pg == GRID_PAGE || cur_pg == SEQ_STEP_PAGE ||
             cur_pg == SEQ_PTC_PAGE || cur_pg == SEQ_EXTSTEP_PAGE);
        const bool md_forward_arrow =
            MD.connected && is_arrow && !arrows_local;
        if (md_forward_arrow) {
            if (!is_release && key_interface.is_key_down(key)) {
                return true; // suppress key repeat
            }
            if (is_release) {
                switch (key) {
                    case MDX_KEY_UP:    CLEAR_BIT64(key_interface.cmd_key_state, key); MD.release_up_arrow();    break;
                    case MDX_KEY_DOWN:  CLEAR_BIT64(key_interface.cmd_key_state, key); MD.release_down_arrow();  break;
                    case MDX_KEY_LEFT:  CLEAR_BIT64(key_interface.cmd_key_state, key); MD.release_left_arrow();  break;
                    case MDX_KEY_RIGHT: CLEAR_BIT64(key_interface.cmd_key_state, key); MD.release_right_arrow(); break;
                    default: break;
                }
            } else {
                switch (key) {
                    case MDX_KEY_UP:    SET_BIT64(key_interface.cmd_key_state, key); MD.hold_up_arrow();    break;
                    case MDX_KEY_DOWN:  SET_BIT64(key_interface.cmd_key_state, key); MD.hold_down_arrow();  break;
                    case MDX_KEY_LEFT:  SET_BIT64(key_interface.cmd_key_state, key); MD.hold_left_arrow();  break;
                    case MDX_KEY_RIGHT: SET_BIT64(key_interface.cmd_key_state, key); MD.hold_right_arrow(); break;
                    default: break;
                }
            }
            return true;
        }

        // YES/NO: transmit to MD AND fire the local key_event so MCL pages
        // (which key off MDX_KEY_YES / MDX_KEY_NO via EVENT_CMD) still see
        // the press. Persists across SPS-mode latching since neither the
        // SpsMode handlers above nor the SPS-mode poll touches these keys.
        if (MD.connected && (key == MDX_KEY_YES || key == MDX_KEY_NO)) {
            if (is_release) {
                if (key == MDX_KEY_YES) MD.release_yes_button();
                else                    MD.release_no_button();
            } else {
                if (key == MDX_KEY_YES) MD.press_yes_button();
                else                    MD.press_no_button();
            }
            // Fall through to key_event below.
        }

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


MCL mcl;
