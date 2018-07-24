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
  md_callbacks.setup();

  note_interface.setup();
  md_exploit.setup();

  mcl_seq.setup();
  A4SysexListener.setup();

  MidiSDSSysexListener.setup();
  midi_setup.cfg_ports();

  // md_setup();
  param1.cur = mcl_cfg.cur_col;
  param2.cur = mcl_cfg.cur_row;
}
MCL mcl;
