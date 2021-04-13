#include "MCLMenus.h"
#include "Project.h"

menu_option_t MENU_OPTIONS[] = {
  // 0: RAM PAGE LINK
  {0, "MONO"}, {1, "STEREO"},
  // 2: MIDI TURBO 1/2
  {0, "1x"}, {1, "2x"}, {2,"4x"}, {3,"8x"},
  // 6: MIDI CLK REC
  {0, "MIDI 1"}, {1, "MIDI 2"},
  // 8: MIDI CLK SEND
  {0, "OFF"}, {1, "MIDI 2"},
  // 10: MIDI FWD
  {0, "OFF"}, {1, "1->2"}, {2, "2->1"},
  // 13: MD TRACK SELECT
  {0, "MAN"}, {1, "AUTO"},
  // 15: MD NORMALIZE
  {0, "OFF"},{1, "AUTO"},
  // 17: MD CTRL CHAN
  {0, "INT"},{17, "OMNI"},
  // 19: MD CHAIN/Slot CHAIN
  {1, "AUT"},{2,"MAN"},{3,"RND"},
  // 22: SYSTEM DISPLAY
  {0, "INT"}, {1, "INT+EXT"},
  // 24: MULTI
  {0, "OFF"}, {1, "ON"},
  // 26: SEQ COPY/CLEAR TRK/PASTE/REVERSE
  {0, "--",}, {1, "TRK"}, {2, "ALL"},
  // 29: SEQ CLEAR LOCKS/CLEAR STEP LOCKS
  {0, "--",}, {1, "LCKS"}, {2, "ALL"},
  // 32: GRID SLOT CLEAR/COPY/PASTE
  {0,"--"}, {1, "YES"},
  // 34: SEQ SHIFT
  {0, "--",}, {1, "L"}, {2, "R"}, {3,"L>ALL"}, {4, "R>ALL"},
  // 39: GRID SLOT APPLY
  {0," "},
  // 40: SEQ SPEED
  {SEQ_SPEED_1X, "1x"}, {SEQ_SPEED_2X , "2x"}, {SEQ_SPEED_3_2X, "3/2x"}, {SEQ_SPEED_3_4X,"3/4x"}, { SEQ_SPEED_1_2X, "1/2x"}, {SEQ_SPEED_1_4X, "1/4x"}, {SEQ_SPEED_1_8X, "1/8x"},
  // 47: SEQ EDIT
  {MASK_PATTERN,"TRIG"}, {MASK_SLIDE,"SLIDE"}, {MASK_LOCK,"LOCK"}, {MASK_MUTE,"MUTE"},
  // 51: GRID
  {0, "A"}, {1, "B"},
  // 53: PIANO ROLL
  {0,"NOTE"},
  // 54: OFF
  {128, "PRG"}, {129, "OFF"}, {130, "LEARN"},
  // 57: PROB
  {1, "L1"}, {2, "L2"}, {3, "L3"}, {4, "L4"}, {5, "L5"}, {6, "L6"}, {7, "L7"}, {8, "L8"}, {9, "P1"}, {10, "P2"}, {11, "P5"}, {12, "P7"}, {13, "P9"},
  // 70: WAV
  {0, "--"}, {1, "SIN"}, {2, "TRI"}, {3, "PUL"}, {4, "SAW"}, {5, "USR"},
  // 76: OSC
  {0, "OSC1"}, {1, "OSC2"}, {2, "OSC3"}, {3, "MIXER"},
  // 80: MIDI_DEVICE
  {0, "GENER"}, {1, "ELEKT"},
};

