#include "MCL.h"

uint8_t in_sysex;
uint8_t in_sysex2;
int8_t curpage;
uint8_t patternswitch = PATTERN_UDEF;

MDPattern pattern_rec;
MDTrack temptrack;


void MCL::setup() {
  DEBUG_PRINTLN("Welcome to MegaCommand Live");
  DEBUG_PRINTLN(VERSION);

  uint8_t charmap[8] = {10, 10, 10, 10, 10, 10, 10, 00};

  LCD.createChar(1, charmap);

  gfx.splashscreen();

  // Initialise Track Routing to Default output (1 & 2)
  //   for (uint8_t i = 0; i < 16; i++) {
  //  MD.setTrackRouting(i,6);
  // }

  // set_midinote_totrack_mapping();

  // Initalise the  Effects Ecnodres

  // mixer_param2.handler = encoder_filter_handle;
  // Setup cfg.uart1_turbo Midi
  // cfg.uart1_turboMidi.setup();
  // Start the SD Card Initialisation.

  // sd_new_project(newprj);
  bool ret = false;

  ret = mcl_sd.load_init();

  // if (!ret) { }

  // MidiClock.mode = MidiClock.EXTERNAL_MIDI;
  md_callbacks.setup();

  note_interface.setup();
  //

  //      GUI.flash_strings_fill("MIDI CLOCK SRC", "MIDI PORT 2");
  md_exploit.setup();
  //   MidiUart.setActiveSenseTimer(290);

  // patternswitch = 7;
  //     int curkit = MD.getCurrentKift(CALLBACK_TIMEOUT);
  //      MD.getBlockingKit(curkit);
  mcl_seq.setup();

  A4SysexListener.setup();

  midi_setup.cfg_ports();
  // md_setup();
  param1.cur = mcl_cfg.cur_col;
  param2.cur = mcl_cfg.cur_row; // turboSetSpeed(1,1);
  // turboSetSpeed(1,2 );
}
MCL mcl;
