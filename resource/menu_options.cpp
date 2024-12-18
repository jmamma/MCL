#include "MCLMenus.h"
#include "SeqTrack.h"

menu_option_t MENU_OPTIONS[] = {
  // 0: RAM PAGE LINK
  {0, "MONO"}, {1, "STEREO"},
  // 2: MIDI TURBO 1/2
  {0, "1x"}, {1, "2x"}, {2,"4x"}, {3,"8x"}, {4,"SHAK"},
  // 7: MIDI CLK REC
  {0, "1"}, {1, "2"}, {2,"USB"},
  // 10: MIDI FWD
  {0, "OFF"}, {1, "2"}, {2, "USB"}, {3, "2 + USB"},
  // 14: MD TRACK SELECT
  {0, "MAN"}, {1, "AUTO"},
  // 16: MD NORMALIZE
  {0, "OFF"},{1, "AUTO"},
  // 18: MD CTRL CHAN
  {0, "--"},{17, "OMNI"},
  // 20: MD CHAIN/Slot CHAIN
  {1, "AUT"},{2,"MAN"},{3,"QUE"},
  // 23: SYSTEM DISPLAY
  {0, "INT"}, {1, "INT+EXT"},
  // 25: MULTI
  {0, "OFF"}, {1, "ON"},
  // 27: SEQ COPY/CLEAR TRK/PASTE/REVERSE
  {0, "--",}, {1, "TRK"}, {2, "ALL"},
  // 30: SEQ CLEAR LOCKS/CLEAR STEP LOCKS
  {0, "--",}, {1, "LCKS"}, {2, "ALL"},
  // 33: GRID SLOT CLEAR/COPY/PASTE
  {0,"--"}, {1, "YES"},
  // 35: SEQ SHIFT
  {0, "--",}, {1, "L"}, {2, "R"}, {3,"L>ALL"}, {4, "R>ALL"},
  // 40: GRID SLOT APPLY
  {0," "},
  // 41: SEQ SPEED
  {SEQ_SPEED_1X, "1x"}, {SEQ_SPEED_2X , "2x"}, {SEQ_SPEED_3_2X, "3/2x"}, {SEQ_SPEED_3_4X,"3/4x"}, { SEQ_SPEED_1_2X, "1/2x"}, {SEQ_SPEED_1_4X, "1/4x"}, {SEQ_SPEED_1_8X, "1/8x"},
  // 48: SEQ EDIT
  {MASK_PATTERN,"TRIG"}, {MASK_SLIDE,"SLIDE"}, {MASK_LOCK,"LOCK"}, {MASK_MUTE,"MUTE"},
  // 52: GRID
  {0, "X"}, {1, "Y"},
  // 54: PIANO ROLL
  {0,"NOTE"},
  // 55: OFF
  {128, "PRG"}, {129, "PB"}, {130, "CHP"}, {131, "OFF"}, {132, "LEARN"},
  // 60: PROB
  {1, "L1"}, {2, "L2"}, {3, "L3"}, {4, "L4"}, {5, "L5"}, {6, "L6"}, {7, "L7"}, {8, "L8"}, {9, "P1"}, {10, "P2"}, {11, "P5"}, {12, "P7"}, {13, "P9"}, {14, "1S"},
  // 74: WAV
  {0, "--"}, {1, "SIN"}, {2, "TRI"}, {3, "PUL"}, {4, "SAW"}, {5, "USR"},
  // 80: OSC
  {0, "OSC1"}, {1, "OSC2"}, {2, "OSC3"}, {3, "MIXER"},
  // 84: MIDI_DEVICE
  {0, "GENER"}, {1, "ELEKT"},
  // 86"
  {0, "OFF"}, {1, "2->2"},
  // 88
  {0, "--"},{17, "OMNI"},
  // 90
  {0, "BASIC"}, {1, "ADV"},
  // 92: MIDI2 FWD
  {0, "OFF"}, {1, "1"}, {2, "USB"}, {3, "1 + USB"},
  // 96: MIDIUSB FWD
  {0, "OFF"}, {1, "1"}, {2, "2"}, {3, "1 + 2"},
  // 100: MIDI CLK SEND
  {0, "OFF"}, {1, "2"}, {2, "USB"}, {3, "2 + USB"},
  // 104: NOTES
  {0, "C"}, {1, "C#"}, {2, "D"}, {3, "D#"}, {4, "E"}, {5, "F"}, {6, "F#"}, {7, "G"}, {8, "G#"}, {9, "A"}, {10, "A#"}, {11, "B"},
  // 116
  {0, "CTRL"},
  // 117
  {0, "A"}, {1, "B"}, {2, "C"}, {3, "D"},
  // 121
  {128, "--"},
  // 122
  {0, "--"}, {1,"PERF"},
  // 124
  {0, "GENER"}, {1,"MD"}
};

