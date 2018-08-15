#include "MCL.h"

uint8_t in_sysex;
uint8_t in_sysex2;
int8_t curpage;
uint8_t patternswitch = PATTERN_UDEF;

MDPattern pattern_rec;
MDTrack temptrack;
MDSysexCallbacks md_callbacks;

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
  md_callbacks.setup();

  note_interface.setup();
  md_exploit.setup();
  md_events.setup();
  mcl_actions.setup();
  mcl_seq.setup();
  A4SysexListener.setup();
  MidiSDSSysexListener.setup();
  midi_setup.cfg_ports();
  for (uint8_t n = 0; n < 16; n++) { SET_BIT32(mcl_cfg.mutes, n); }
  mute_page.midi_events.setup_callbacks();

  param1.cur = mcl_cfg.cur_col;
  param2.cur = mcl_cfg.cur_row;

  if (mcl_cfg.display_mirror == 1) {
#ifndef DEBUGMODE
    Serial.begin(250000);
    GUI.display_mirror = true;
#endif
  }
}
MCL mcl;
