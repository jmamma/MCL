#include "MenuTypes.h"
#include "MCLMenuDefines.h"
#include "MCLMemory.h"
#include "MidiSetup.h"

/****
  Menu Format:
  Name
  Item of (name min range nropts dstvar_id page_id rowfunc_id opts_offset)
    - opts_offset >= 192 means custom options table. see MenuBase::set_custom_options
  ExitFunc
  ExitPage
 ***/


menu_t<boot_menu_page_N> boot_menu_layout = {
    "BOOT",
    {
        //               m  r  n  d  p  f  o
        {"OS UPGRADE",  0, 0, 0, 0, NULL_PAGE, 27, 0},
#if defined(__AVR__)
        {"DFU MODE",    0, 0, 0, 0, NULL_PAGE, 26, 0},
#endif
        {"USB DISK",    0, 0, 0, 0, NULL_PAGE, 28, 0},
        {"EXIT",        0, 0, 0, 0, NULL_PAGE, 29, 0},
    },
    0
};


menu_t<start_menu_page_N> start_menu_layout = {
    "PROJECT",
    {
        //               m  r  n  d  p  f  o
        {"LOAD PROJECT", 0, 0, 0, 0, LOAD_PROJ_PAGE, 0, 0},
        {"NEW PROJECT",  0, 0, 0, 0, NULL_PAGE, 2, 0},
    },
    0
};

menu_t<system_menu_page_N> system_menu_layout = {
    "CONFIG",
    {
        //               m  r  n  d  p  f  o
        {"LOAD PROJECT", 0, 0, 0, 0, LOAD_PROJ_PAGE, 0, 0},
        //{"CONV PROJECT", 0, 0, 0, 0, 2, 0, 0},
        {"MIDI",         0, 0, 0, 0, MIDI_CONFIG_PAGE, 0, 0},
        {"DRIVER 1",     0, 0, 0, 0, NULL_PAGE, 32, 0},
        {"DRIVER 2",     0, 0, 0, 0, NULL_PAGE, 33, 0},
        {"PAGE SETUP",   0, 0, 0, 0, AUX_CONFIG_PAGE, 0, 0},
        {"SYSTEM",       0, 0, 0, 0, MCL_CONFIG_PAGE, 0, 0},
    },
    0
};

menu_t<aux_config_page_N> auxconfig_menu_layout = {
    "PAGE",
    {
        //           m  r  n  d  p  f  o

        {"GRID ENCOD:", 0, 2, 2, 62, NULL_PAGE, 0, 120},
    },
    1
};

menu_t<midi_config_page_N> midiconfig_menu_layout = {
    "MIDI",
    {
        {"DEVICES", 0, 0, 0, 0, MIDIDEVICE_MENU_PAGE, 0, 0},
#if defined(PLATFORM_TBD)
        {"TURBO", 0, 0, 0, 0, MIDIPORT_MENU_PAGE, 0, 0},
#else
        {"PORTS", 0, 0, 0, 0, MIDIPORT_MENU_PAGE, 0, 0},
#endif
        {"SYNC",  0, 0, 0, 0, MIDICLOCK_MENU_PAGE, 0, 0},
        {"ROUTING", 0, 0, 0, 0, MIDIROUTE_MENU_PAGE, 0, 0},
        {"CONTROLLER", 0, 0, 0, 0, MIDIGENERIC_MENU_PAGE, 0 ,0},
        {"MD MIDI", 0, 0, 0, 0, MIDIMACHINEDRUM_MENU_PAGE, 0 ,0},
        {"PROGRAM", 0, 0, 0, 0, MIDIPROGRAM_MENU_PAGE, 0, 0},
    },
    0
};

#if defined(__AVR__)
  #define TURBO_RANGE 4
#else
  #define TURBO_RANGE 6
#endif

#if defined(PLATFORM_TBD)
  #define GRIDX_DEVICE_RANGE 3
  #define GRIDX_DEVICE_OPTIONS 3
  #define GRIDX_PORT_MIN 0
  #define GRIDX_PORT_RANGE 3
  #define GRIDX_PORT_OPTIONS 3
  #define GRIDX_PORT_OPTIONS_OFFSET 136
  #define GRIDY_DEVICE_MIN 0
  #define GRIDY_DEVICE_RANGE 4
  #define GRIDY_DEVICE_OPTIONS 4
  #define GRIDY_DEVICE_OPTIONS_OFFSET 139
  #define GRIDY_PORT_MIN 0
  #define GRIDY_PORT_RANGE 3
  #define GRIDY_PORT_OPTIONS 3
  #define GRIDY_PORT_OPTIONS_OFFSET 143
  #define MIDI_CLOCK_SOURCE_OPTIONS_OFFSET 146
  #define LFO_MULT_OPTIONS_OFFSET 150
#else
  #define GRIDX_DEVICE_RANGE 2
  #define GRIDX_DEVICE_OPTIONS 2
  #define GRIDX_PORT_MIN 0
  #define GRIDX_PORT_RANGE 2
  #define GRIDX_PORT_OPTIONS 2
  #define GRIDX_PORT_OPTIONS_OFFSET 135
  #define GRIDY_DEVICE_MIN 1
  #define GRIDY_DEVICE_RANGE 4
  #define GRIDY_DEVICE_OPTIONS 3
  #define GRIDY_DEVICE_OPTIONS_OFFSET 137
  #define GRIDY_PORT_MIN 1
  #define GRIDY_PORT_RANGE 3
  #define GRIDY_PORT_OPTIONS 2
  #define GRIDY_PORT_OPTIONS_OFFSET 140
  #define MIDI_CLOCK_SOURCE_OPTIONS_OFFSET 7
  #define LFO_MULT_OPTIONS_OFFSET 142
#endif

menu_t<mididevice_menu_page_N> mididevice_menu_layout = {
    "DEVICES",
    {
        {"GRID X", 0, 0, 0, 0, GRIDX_MENU_PAGE, 0, 0},
        {"GRID Y", 0, 0, 0, 0, GRIDY_MENU_PAGE, 0, 0},
    },
    24
};

menu_t<gridx_menu_page_N> gridx_menu_layout = {
    "GRID X",
    {
        {"DEVICE:", 0, GRIDX_DEVICE_RANGE, GRIDX_DEVICE_OPTIONS,
         66, NULL_PAGE, 0, 133},
        {"PORT:", GRIDX_PORT_MIN, GRIDX_PORT_RANGE, GRIDX_PORT_OPTIONS,
         67, NULL_PAGE, 0, GRIDX_PORT_OPTIONS_OFFSET},
    },
    24
};

menu_t<gridy_menu_page_N> gridy_menu_layout = {
    "GRID Y",
    {
        {"DEVICE:", GRIDY_DEVICE_MIN, GRIDY_DEVICE_RANGE, GRIDY_DEVICE_OPTIONS,
         68, NULL_PAGE, 0, GRIDY_DEVICE_OPTIONS_OFFSET},
        {"PORT:", GRIDY_PORT_MIN, GRIDY_PORT_RANGE, GRIDY_PORT_OPTIONS,
         69, NULL_PAGE, 0, GRIDY_PORT_OPTIONS_OFFSET},
    },
    24
};

menu_t<midiport_menu_page_N> midiport_menu_layout = {
#if defined(PLATFORM_TBD)
    "TURBO",
    {
        {"MIDI 1:", 0, TURBO_RANGE, TURBO_RANGE, 2, NULL_PAGE, 0, 54},
        {"MIDI 2:", 0, TURBO_RANGE, TURBO_RANGE, 3, NULL_PAGE, 0, 54},
        {"USB:",    0, TURBO_RANGE, TURBO_RANGE, 55, NULL_PAGE, 0, 54},
    },
#else
    "PORTS",
    {
        {"PORT 1", 0, 0, 0, 0, PORT1_MENU_PAGE,   0, 0},
        {"PORT 2", 0, 0, 0, 0, PORT2_MENU_PAGE,   0, 0},
        {"USB",    0, 0, 0, 0, USBPORT_MENU_PAGE, 0, 0},
    },
#endif
    24
};

menu_t<port1_menu_page_N> port1_menu_layout = {
    "PORT 1",
    {
        {"TURBO:",  0, TURBO_RANGE, TURBO_RANGE, 2, NULL_PAGE, 0, 54},
    },
    24
};

menu_t<port2_menu_page_N> port2_menu_layout = {
    "PORT 2",
    {
        {"TURBO:",  0, TURBO_RANGE, TURBO_RANGE, 3, NULL_PAGE, 0, 54},
    },
    24
};

menu_t<usbport_menu_page_N> usbport_menu_layout = {
    "USB",
    {
        {"TURBO:",  0, TURBO_RANGE, TURBO_RANGE, 55, NULL_PAGE, 0, 54},
    },
    24
};

menu_t<midiprogram_menu_page_N> midiprogram_menu_layout = {
    "PROGRAM",
    {
        {"PRG MODE:", 0, 2, 2, 49, NULL_PAGE, 0, 88},
        {"PRG IN:", 0, 18, 2, 47, NULL_PAGE, 0, 86},
        {"PRG OUT:", 0, 17, 2, 48, NULL_PAGE, 0, 86},
    },
    24
};


menu_t<midiclock_menu_page_N> midiclock_menu_layout = {
    "SYNC",
    {
#if defined(PLATFORM_TBD)
        {"CLOCK SRC:",   0, MIDI_CLOCK_SOURCE_COUNT, MIDI_CLOCK_SOURCE_COUNT,
         5, NULL_PAGE, 0, MIDI_CLOCK_SOURCE_OPTIONS_OFFSET},
        {"TRANS SRC:",   0, MIDI_CLOCK_SOURCE_COUNT, MIDI_CLOCK_SOURCE_COUNT,
         53, NULL_PAGE, 0, MIDI_CLOCK_SOURCE_OPTIONS_OFFSET},
#else
        {"CLOCK RECV:",  0, MIDI_CLOCK_SOURCE_COUNT, MIDI_CLOCK_SOURCE_COUNT,
         5, NULL_PAGE, 0, MIDI_CLOCK_SOURCE_OPTIONS_OFFSET},
        {"TRANS RECV:",  0, MIDI_CLOCK_SOURCE_COUNT, MIDI_CLOCK_SOURCE_COUNT,
         53, NULL_PAGE, 0, MIDI_CLOCK_SOURCE_OPTIONS_OFFSET},
#endif
        {"CLOCK SEND:", 0, 4, 4, 6, NULL_PAGE, 0, 98},
        {"TRANS SEND:",  0, 4, 4, 54, NULL_PAGE, 0, 98},
    },
    24
};

menu_t<midiroute_menu_page_N> midiroute_menu_layout = {
    "ROUTE",
    {
        //            m  r  n  d  p  f  o
        {"MIDI 1 FWD:", 0, 4, 4, 7, NULL_PAGE, 0, 10},
        {"MIDI 2 FWD:", 0, 4, 4, 51, NULL_PAGE, 0, 90},
        {"USB FWD:", 0, 4, 4, 52, NULL_PAGE, 0, 94},

    },
    24
};

menu_t<midimachinedrum_menu_page_N> midimachinedrum_menu_layout = {
    "MD MIDI",
    {
        //              m  r   n  d  p  f  o
        {"CHRO CHAN:",  0, 17, 2, 9, NULL_PAGE, 0, 18},
        {"POLY CHAN:",  0, 17, 2, 46, NULL_PAGE, 0, 86},
        {"TRIG CHAN:",   0, 17, 2, 57, NULL_PAGE, 0, 18},
    },
    24
};

menu_t<midigeneric_menu_page_N> midigeneric_menu_layout = {
    "CONTROL",
    {
        //              m  r   n  d  p  f  o
        {"CTRL PORT:", 1, 4, 4, 56, NULL_PAGE, 0, 98},
        {"NOTE FWD:",  0, 2, 2, 64, NULL_PAGE, 0, 25},
        {"CC FWD:", 0, 2, 2, 11, NULL_PAGE, 0, 25},
        {"MUTE CC:",  0, 129, 1, 60, NULL_PAGE, 0, 119},
    },
    24
};


menu_t<md_config_page_N> mdconfig_menu_layout = {
    "MD",
    {
        //              m  r   n  d  p  f  o
        {"IMPORT",      0, 0,  0, 0, MD_IMPORT_PAGE, 0, 0},
        {"RAM LINK:",   0, 2,  2, 1, NULL_PAGE, 0, 0},
        {"NORMALIZE:",  0, 2,  2, 8, NULL_PAGE, 0, 16},
    },
    1
};

menu_t<md_import_page_N> mdimport_menu_layout = {
    "MD",
    {
        //         m  r       n  d  p  f  o
        {"SRC: ",  0, 128, 128, 43, NULL_PAGE, 0, 192},
        {"DEST: ", 0, 128, 128, 44, NULL_PAGE, 0, 192},
        {"COUNT:", 1, 129,  0,  45, NULL_PAGE, 0, 0},
        {"RUN",    0,   0,  0,   0, NULL_PAGE, 25, 0},
    },
    0
};

menu_t<mcl_config_page_N> mclconfig_menu_layout = {
    "SYSTEM",
    {
        //           m  r  n  d   p  f  o
        {"DISPLAY:", 0, 2, 2, 13, NULL_PAGE, 0, 23},
        {"PROJ CFG:", 0, 2, 2, 72, NULL_PAGE, 0, 25},
    },
    1
};

menu_t<file_menu_page_N> file_menu_layout = {
    "FILE",
    {
        //            m  r  n  d  p  f  o
        {"CANCEL",    0, 0, 0, 0, NULL_PAGE, 0, 0},
        {"NEW DIR",   0, 0, 0, 0, NULL_PAGE, 0, 0},
        {"RENAME",    0, 0, 0, 0, NULL_PAGE, 0, 0},
        {"MOVE",      0, 0, 0, 0, NULL_PAGE, 0, 0},
        {"CLONE",      0, 0, 0, 0, NULL_PAGE, 0, 0},
        {"VERS",      0, 0, 0, 0, NULL_PAGE, 0, 0},
        {"DELETE",    0, 0, 0, 0, NULL_PAGE, 0, 0},
        {"RECV ALL",  0, 0, 0, 0, NULL_PAGE, 0, 0},
        {"SEND ALL",  0, 0, 0, 0, NULL_PAGE, 0, 0},
    },
    0
};

menu_t<seq_menu_page_N> seq_menu_layout = {
    "SEQ",
    {
        //              m  r                    n                    d   p  f   o
        {"TRACK SEL:",  1, 17,                  0,                   14, NULL_PAGE,  3,  0},
        {"DEVICE:",     0, 2,                   2,                   50, NULL_PAGE,  0,  192},
        {"EDIT:",       0, 4,                   4,                   15, NULL_PAGE,  4,  48},
        {"EDIT:",       0, 1 + NUM_LOCKS,       1,                   16, NULL_PAGE,  0,  122},
        {"CC:",         0, 133,                 5,                   17, NULL_PAGE,  0,  2},
        {"SLIDE:",      0, 2,                   2,                   18, NULL_PAGE,  0,  25},
        {"ARPEGGIATOR", 0, 0,                   0,                   0,  ARP_PAGE, 0,  0},
        {"KEY:",        0, 12,                  12,                  19, NULL_PAGE,  0,  102},
        {"VEL:",        0, 128,                 0,                   20, NULL_PAGE,  0,  0},
        {"COND:",       1, NUM_TRIG_CONDITIONS + 1, NUM_TRIG_CONDITIONS + 1, 21, NULL_PAGE,  0,  60},
        {"SPEED:",      0, 7,                   7,                   22, NULL_PAGE,  5,  41},
        {"LENGTH:",     1, 65,                 0,                   23, NULL_PAGE,  6,  0},
        {"LENGTH:",     2, 129,                 0,                   23, NULL_PAGE,  6,  0},
        {"CHANNEL:",    1, 17,                  0,                   24, NULL_PAGE,  7,  0},
        {"COPY:  ",     0, 3,                   3,                   25, NULL_PAGE,  8,  27},
        {"CLEAR:",      0, 3,                   3,                   26, NULL_PAGE,  9,  27},
        {"CLEAR:",      0, 3,                   3,                   26, NULL_PAGE,  10, 30},
        {"PASTE:",      0, 3,                   3,                   27, NULL_PAGE,  11, 27},
        {"SHIFT:",      0, 5,                   5,                   28, NULL_PAGE,  12, 35},
        {"REVERSE:",    0, 3,                   3,                   29, NULL_PAGE,  13, 27},
        {"TRAN:",       0, 50,                  51,                  63, NULL_PAGE,  31, 193},
        {"POLYPHONY",   0, 0,                   0,                   0,  POLY_PAGE,  0,  0},
        {"QUANT:",      0, 2,                   2,                   42, NULL_PAGE,  0,  25},
        {"CC REC:",     0, 2,                   2,                   30, NULL_PAGE,  0,  25},
        {"SOUND",       0, 0,                   0,                   0,  SOUND_BROWSER, 0,  0},
        {"LFO MULT:",   0, 8,                   8,                   70, NULL_PAGE,  0,  LFO_MULT_OPTIONS_OFFSET},
    },
    14
};

menu_t<grid_slot_page_N> slot_menu_layout = {
    "Slot",
    {
        //          m  r    n  d   p  f   o
        {"GRID: ",  0, 2,   2, 31, NULL_PAGE, 0,  52},
        // for non-ext tracks
        {"LEN:   ",   1, 65,  0, 39, NULL_PAGE, 0,  0},
        // for ext tracks
        {"LEN:   ",   1, 129, 0, 39, NULL_PAGE, 0,  0},
        {"LOOP: ",  0, 64,  0, 33, NULL_PAGE, 0,  0},
        // o=128, generate the table on-demand
        {"JUMP: ", 0, 128, 128, 34, NULL_PAGE, 0, 192},
        {"SOUND:",  0, 2,   2, 71, NULL_PAGE, 0,  25},
        {"CLEAR:",  0, 2,   2, 36, NULL_PAGE, 0,  33},
        {"COPY:  ", 0, 2,   2, 37, NULL_PAGE, 0,  33},
        {"PASTE:",  0, 2,   2, 38, NULL_PAGE, 0,  33},
        {"RENAME",  0, 0,   0, 0,  NULL_PAGE, 20, 0},
    },
    21
};

menu_t<wavdesign_menu_page_N> wavdesign_menu_layout = {
    "",
    {
        //           m  r  n  d   p  f   o
        {"EDIT:",    0, 4, 4, 40, NULL_PAGE, 0,  80},
        {"WAV:",     0, 6, 6, 41, NULL_PAGE, 0,  74},
        {"TRANSFER", 0, 0, 0, 0,  NULL_PAGE, 22, 0},
    },
    23
};

menu_t<perf_menu_page_N> perf_menu_layout = {
    "",
    {
        //           m  r  n  d   p  f   o

        {"CTRL SEL:",0,  4, 4, 59, NULL_PAGE, 0,  115},
        {"RENAME",  0, 0,   0, 0,  NULL_PAGE, 30, 0},
      //  {"PARAM:",    0, 17, 1, 58, NULL_PAGE, 0,  116},
    },
    0
};
