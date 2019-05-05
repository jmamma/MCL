#include "MCL.h"

int8_t curpage;
uint8_t patternswitch = PATTERN_UDEF;


void MCL::setup() {
  DEBUG_PRINTLN("Welcome to MegaCommand Live");
  DEBUG_PRINTLN(VERSION);

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

  gfx.splashscreen();
  // if (!ret) { }

  ret = mcl_sd.load_init();
  DEBUG_PRINTLN("tempo:");
  DEBUG_PRINTLN(mcl_cfg.tempo);
  MidiClock.setTempo(mcl_cfg.tempo);

  note_interface.setup();
  md_exploit.setup();

  MD.midi_events.enable_live_kit_update();

  mcl_actions.setup();
  mcl_seq.setup();
  MDSysexListener.setup(&Midi);
  A4SysexListener.setup(&Midi2);
  MidiSDSSysexListener.setup(&Midi);
  midi_setup.cfg_ports();
  for (uint8_t n = 0; n < 16; n++) { SET_BIT32(mcl_cfg.mutes, n); }
  mute_page.midi_events.setup_callbacks();
  GUI.addTask(&grid_task);

  if (mcl_cfg.display_mirror == 1) {
#ifndef DEBUGMODE
    Serial.begin(250000);
    GUI.display_mirror = true;
#endif
  }
    if (mcl_cfg.screen_saver == 1) {
  GUI.use_screen_saver = true;
  }
  else {
  GUI.use_screen_saver = false;
  }

  DEBUG_PRINTLN("Track sizes:");
  DEBUG_PRINTLN(sizeof(A4Track));
  DEBUG_PRINTLN(sizeof(MDTrack));
  DEBUG_PRINTLN(sizeof(MDSeqTrackData));
  DEBUG_PRINTLN(sizeof(GridTrack) + sizeof(MDSeqTrackData) + sizeof(MDMachine));
}
MCL mcl;
