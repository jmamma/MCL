#include "MCL.h"

MCLEncoder options_param1(0, 5, ENCODER_RES_SYS);
MCLEncoder options_param2(0, 3, ENCODER_RES_SYS);
MCLSystemPage system_page(&options_param1, &options_param2);

uint8_t in_sysex = 0;
uint8_t in_sysex2 = 0;
int8_t curpage = 0;

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

  // mixer_param2.handler = encoder_filter_handle;
  // Setup cfg.uart1_turbo Midi
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
