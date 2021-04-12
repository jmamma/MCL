#include "MCL_impl.h"
#include "ResourceManager.h"

int8_t curpage;
uint8_t patternswitch = PATTERN_UDEF;

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

  bool ret = false;
  delay(100);
  ret = mcl_sd.sd_init();
#ifdef OLED_DISPLAY
  gfx.init_oled();
#endif
  if (!ret) {
#ifdef OLED_DISPLAY
    oled_display.print("SD CARD ERROR :-(");
    oled_display.display();
#else
    GUI.flash_strings_fill("SD CARD ERROR", "");
#endif

    delay(2000);
    return;
  }

  R.Clear();
  R.use_icons_boot();
  gfx.splashscreen(R.icons_boot->mcl_logo_bitmap);
  // if (!ret) { }
  text_input_page.no_escape = true;
  ret = mcl_sd.load_init();
  text_input_page.no_escape = false;
  if (ret) {
    GUI.setPage(&grid_page);
  }

  DEBUG_PRINTLN(F("tempo:"));
  DEBUG_PRINTLN(mcl_cfg.tempo);
  MidiClock.setTempo(mcl_cfg.tempo);

  note_interface.setup();
  // md_exploit.setup();

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

#ifdef MEGACOMMAND
  MidiSDSSysexListener.setup(&Midi);
#endif

  midi_setup.cfg_ports();
  GUI.addTask(&grid_task);
  GUI.addTask(&midi_active_peering);

  GUI.addEventHandler((event_handler_t)&mcl_handleEvent);

  if (mcl_cfg.display_mirror == 1) {
#ifndef DEBUGMODE
#ifdef OLED_DISPLAY
    oled_display.textbox("DISPLAY ", "MIRROR");
#endif
    Serial.begin(250000);
    GUI.display_mirror = true;
#endif
  }
  if (mcl_cfg.screen_saver == 1) {
    GUI.use_screen_saver = true;
  } else {
    GUI.use_screen_saver = false;
  }

  DEBUG_PRINTLN(F("Track sizes:"));
#ifdef EXT_TRACKS
  DEBUG_PRINTLN(sizeof(A4Track));
#endif
  DEBUG_PRINTLN(sizeof(MDTrack));
  DEBUG_PRINTLN(sizeof(MDSeqTrackData));
  DEBUG_PRINTLN(sizeof(GridTrack) + sizeof(MDSeqTrackData) + sizeof(MDMachine));
}

bool mcl_handleEvent(gui_event_t *event) {

  DEBUG_PRINTLN("RECEV EVENT");
  DEBUG_PRINTLN(event->source);
  if (EVENT_CMD(event)) {
    uint8_t key = event->source - 64;
    DEBUG_PRINTLN(key);
    DEBUG_PRINTLN(MDX_KEY_REC);
    if (event->mask == EVENT_BUTTON_PRESSED) {
      switch (key) {
      case MDX_KEY_REC: {
        if (GUI.currentPage() != &seq_step_page &&
            GUI.currentPage() != &seq_ptc_page &&
            GUI.currentPage() != &seq_param_page) {
          GUI.setPage(&seq_step_page);
        } else {
          if (seq_step_page.recording) {
            seq_step_page.recording = 0;
            MD.set_rec_mode(1);
          } else {
            if (GUI.currentPage() == &seq_step_page) {
              MD.set_rec_mode(0);
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
        opt_copy = 2;
        opt_copy_track_handler();
        break;
      }
      case MDX_KEY_PASTE: {
        if (GUI.currentPage() == &seq_step_page)
          break;
        opt_paste = 2;
        opt_paste_track_handler();
        break;
      }
      case MDX_KEY_CLEAR: {
        if (GUI.currentPage() == &seq_step_page)
          break;
        opt_clear = 2;
        opt_clear_track_handler();
      }
      }
    }
    if (event->mask == EVENT_BUTTON_RELEASED) {
      switch (key) {
      case MDX_KEY_REC: {
        if (GUI.currentPage() != &seq_step_page &&
            seq_step_page.recording == 0) {
          page_select_page.md_prepare();
          GUI.setPage(&grid_page);
        }
        return true;
      }
      case MDX_KEY_REALTIME: {
        seq_step_page.recording = 0;
        MD.set_rec_mode(1);
        clearLed2();
        return true;
      }
      }
    }
  }
}

MCL mcl;
