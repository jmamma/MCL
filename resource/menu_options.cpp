#include "MenuTypes.h"
#include "SeqDefines.h"

menu_option_t MENU_OPTIONS[] = {
  // 0: RAM PAGE LINK
  {0, "MONO"}, {1, "STEREO"},
  // 2: OFF
  {128, "PRG"}, {129, "PB"}, {130, "CHP"}, {131, "OFF"}, {132, "LEARN"},
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
  // 54: MIDI TURBO 1/
  #if defined(__AVR__)
  {0, "1x"}, {1, "2x"}, {2,"4x"}, {3,"8x"}, {4,"--"}, {5,"--"},
  #else
  {0, "1x"}, {1, "2x"}, {2,"4x"}, {3,"6.7x"}, {4,"8x"}, {5,"10x"},
  #endif
  // 60: PROB
  {1, "L1"}, {2, "L2"}, {3, "L3"}, {4, "L4"}, {5, "L5"}, {6, "L6"}, {7, "L7"}, {8, "L8"}, {9, "P1"}, {10, "P2"}, {11, "P5"}, {12, "P7"}, {13, "P9"}, {14, "1S"},
  // 74: WAV
  {0, "--"}, {1, "SIN"}, {2, "TRI"}, {3, "PUL"}, {4, "SAW"}, {5, "USR"},
  // 80: OSC
  {0, "OSC1"}, {1, "OSC2"}, {2, "OSC3"}, {3, "MIXER"},
  // 84:
  {0, "OFF"}, {1, "CTRL->2"},
  // 86
  {0, "--"},{17, "OMNI"},
  // 88
  {0, "BASIC"}, {1, "ADV"},
  // 90: MIDI2 FWD
  {0, "OFF"}, {1, "1"}, {2, "USB"}, {3, "1 + USB"},
  // 94: MIDIUSB FWD
  {0, "OFF"}, {1, "1"}, {2, "2"}, {3, "1 + 2"},
  // 98: MIDI CLK SEND
  {0, "OFF"}, {1, "2"}, {2, "USB"}, {3, "2 + USB"},
  // 102: NOTES
  {0, "C"}, {1, "C#"}, {2, "D"}, {3, "D#"}, {4, "E"}, {5, "F"}, {6, "F#"}, {7, "G"}, {8, "G#"}, {9, "A"}, {10, "A#"}, {11, "B"},
  // 114
  {0, "CTRL"},
  // 115
  {0, "A"}, {1, "B"}, {2, "C"}, {3, "D"},
  // 119
  {128, "--"},
  // 120
  {0, "--"}, {1,"PERF"},
  // 122: PIANO ROLL
  {0,"NOTE"},
  // 123: PORT 1 device
  {0, "GENER"}, {1, "MD"}, {2, "OFF"},
  // 126: PORT 2 device
  {0, "GENER"}, {1, "ELEKT"}, {2, "OFF"},
  // 129: USB device
  {0, "OFF"}, {1, "MD"}, {2, "ELEKT"}, {3, "GENER"},
#if defined(PLATFORM_TBD)
  // 133: GRID X device
  {0, "OFF"}, {1, "MD"}, {2, "TBD"},
  // 136: GRID X port
  {0, "INT"}, {1, "1"}, {2, "USB"},
  // 139: GRID Y device
  {0, "TBD"}, {1, "GENER"}, {2, "ELEKT"}, {3, "OFF"},
  // 143: GRID Y port
  {0, "INT"}, {1, "2"}, {2, "USB"},
#else
  // 133: GRID X device
  {0, "OFF"}, {1, "MD"},
  // 135: GRID X port
  {0, "1"}, {1, "USB"},
  // 137: GRID Y device
  {1, "GENER"}, {2, "ELEKT"}, {3, "OFF"},
  // 140: GRID Y port
  {1, "2"}, {2, "USB"},
#endif
};
