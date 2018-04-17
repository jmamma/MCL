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

  gfx.splashscreen();

  bool ret = false;

  ret = mcl_sd.load_init();

  // if (!ret) { }
  md_callbacks.setup();

  note_interface.setup();
  md_exploit.setup();

  mcl_seq.setup();

  A4SysexListener.setup();

  midi_setup.cfg_ports();
  
  // md_setup();
  param1.cur = mcl_cfg.cur_col;
  param2.cur = mcl_cfg.cur_row;

}
MCL mcl;
