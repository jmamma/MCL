#include "MCL.h"

uint8_t in_sysex = 0;
uint8_t in_sysex2 = 0;
int8_t curpage = 0;

float frames_fps = 10;
uint16_t frames = 0;
uint16_t frames_startclock;

void MCL::setup() {
  Serial.begin(9600);
  DEBUG_PRINTLN("Welcome to MegaCommand Live");
  DEBUG_PRINTLN(VERSION);

  uint8_t charmap[8] = {10, 10, 10, 10, 10, 10, 10, 00};

  LCD.createChar(1, charmap);

  // Enable callbacks, and disable some of the ones we don't want to use.

  // MDTask.setup();
  // MDTask.verbose = false;
  // MDTask.autoLoadKit = false;
  // MDTask.reloadGlobal = false;

  // GUI.addTask(&MDTask);

  // Create a mdHandler object to handle callbacks.

  MDSysexCallbacks mdHandler;
  mdHandler.setup();

  // int temp = MD.getCurrentKit(50);

  // Load the splashscreen
  gfx.splashscreen();

  // Initialise Track Routing to Default output (1 & 2)
  //   for (uint8_t i = 0; i < 16; i++) {
  //  MD.setTrackRouting(i,6);
  // }

  // set_midinote_totrack_mapping();

  // Initalise the  Effects Ecnodres

  param2.handler = encoder_param2_handle;
  param3.handler = encoder_fx_handle;
  param3.effect = MD_FX_ECHO;
  param3.fxparam = MD_ECHO_TIME;
  param4.handler = encoder_fx_handle;
  param4.effect = MD_FX_ECHO;
  param4.fxparam = MD_ECHO_FB;

  trackinfo_param4.handler = octave_handler;
  mixer_param1.handler = encoder_level_handle;
  mixer_param2.handler = encoder_level_handle;
  trackinfo_param3.handler = pattern_len_handler;
  trackinfo_param2.handler = ptc_root_handler;
  // mixer_param2.handler = encoder_filter_handle;
  // Setup cfg.uart1_turbo Midi
  frames_startclock = slowclock;
  // cfg.uart1_turboMidi.setup();
  // Start the SD Card Initialisation.

  // sd_new_project(newprj);
  bool ret = false;

  ret = mcl_sd.load_init();

  // if (!ret) { }

  // MidiClock.mode = MidiClock.EXTERNAL_MIDI;


  NoteInteface.setup()
  //

  //      GUI.flash_strings_fill("MIDI CLOCK SRC", "MIDI PORT 2");
  md_exploit_callbacks.setup();
  //   MidiUart.setActiveSenseTimer(290);

  // patternswitch = 7;
  //     int curkit = MD.getCurrentKift(CALLBACK_TIMEOUT);
  //      MD.getBlockingKit(curkit);
  mcl_seq.setup();

  A4SysexListener.setup();

  sei();
  midi_setup.cfg_ports();
  // md_setup();
  param1.cur = cfg.cur_col;
  param2.cur = cfg.cur_row; // turboSetSpeed(1,1);
  // turboSetSpeed(1,2 );
}
MCL mcl;
