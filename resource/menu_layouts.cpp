#include "MCLMenus.h"
#include "Project.h"
#include "MCL_impl.h"

/****
  Menu Format:
  Name
  Item of (name min range nropts dstvar_id page_id rowfunc_id opts_offset)
    - opts_offset >= 128 means custom options table. see MenuBase::set_custom_options
  ExitFunc
  ExitPage
 ***/

menu_t<4> boot_menu_layout = {
    "BOOT",
    {
        //               m  r  n  d  p  f  o
        {"OS UPGRADE",  0, 0, 0, 0, 0, 27, 0},
        {"DFU MODE",  0, 0, 0, 0, 0, 26, 0},
        {"USB DISK", 0, 0, 0, 0, 0, 28, 0},
        {"EXIT", 0, 0, 0, 0, 0, 29, 0},
    },
    0, 0
};


menu_t<2> start_menu_layout = {
    "PROJECT",
    {
        //               m  r  n  d  p  f  o
        {"LOAD PROJECT", 0, 0, 0, 0, 1, 0, 0},
        {"NEW PROJECT",  0, 0, 0, 0, 0, 2, 0},
    },
    0, 0
};

menu_t<6> system_menu_layout = {
    "CONFIG",
    {
        //               m  r  n  d  p  f  o
        {"LOAD PROJECT", 0, 0, 0, 0, 1, 0, 0},
        //{"CONV PROJECT", 0, 0, 0, 0, 2, 0, 0},
        {"NEW PROJECT",  0, 0, 0, 0, 0, 2, 0},
        {"MIDI",         0, 0, 0, 0, 3, 0, 0},
        {"MACHINEDRUM",  0, 0, 0, 0, 4, 0, 0},
        {"AUX PAGES",    0, 0, 0, 0, 6, 0, 0},
        {"SYSTEM",       0, 0, 0, 0, 7, 0, 0},
    },
    0, 0
};

menu_t<1> auxconfig_menu_layout = {
    "AUX PAGES",
    {
        //           m  r  n  d  p  f  o
        {"RAM Page" ,0, 0, 0, 0, 8, 0, 0},
    },
    0, 0,
};

menu_t<1> rampage1_menu_layout = {
    "RAM PAGE",
    {
        //        m  r  n  d  p  f  o
        {"LINK:", 0, 2, 2, 1, 0, 0, 0},
    },
    0, 0
};

menu_t<5> midiconfig_menu_layout = {
    "MIDI",
    {
        {"PORT CONFIG", 0, 0, 0, 0, 12, 0, 0},
        {"SYNC",  0, 0, 0, 0, 14, 0, 0},
        {"ROUTING", 0, 0, 0, 0, 15, 0, 0},
        {"PROGRAM", 0, 0, 0, 0, 13, 0, 0},
        {"MD MIDI", 0, 0, 0, 0, 16, 0 ,0},
    },
    0, 0
};

menu_t<5> midiport_menu_layout = {
    "PORTS",
    {
        {"TURBO 1:",  0, 4, 4, 2, 0, 0, 2},
        {"TURBO 2:",  0, 4, 4, 3, 0, 0, 2},
        {"TURBO USB:", 0, 4, 4, 55, 0 , 0, 2},
        {"DRIVER 2:", 0, 2, 2, 4, 0, 0, 84},
        {"CTRL PORT:", 1, 4, 4, 56, 0, 0, 100},
    },
    24, 0
};

menu_t<3> midiprogram_menu_layout = {
    "PROGRAM",
    {
        {"PRG MODE:", 0, 2, 2, 49, 0, 0, 90},
        {"PRG IN:", 0, 18, 2, 47, 0, 0, 88},
        {"PRG OUT:", 0, 17, 2, 48, 0, 0, 88},
    },
    24, 0
};


menu_t<4> midiclock_menu_layout = {
    "SYNC",
    {
        {"CLOCK RECV:",  0, 3, 3, 5, 0, 0, 7},
        {"TRANS RECV:",  0, 3, 3, 53, 0, 0, 7},
        {"CLOCK SEND:", 0, 4, 4, 6, 0, 0, 100},
        {"TRANS SEND:",  0, 4, 4, 54, 0, 0, 100},
    },
    24, 0
};

menu_t<4> midiroute_menu_layout = {
    "ROUTE",
    {
        //            m  r  n  d  p  f  o
        {"MIDI 1 FWD:", 0, 4, 4, 7, 0, 0, 10},
        {"MIDI 2 FWD:", 0, 4, 4, 51, 0, 0, 92},

        {"USB FWD:", 0, 4, 4, 52, 0, 0, 96},

        {"CC LOOP:", 0, 2, 2, 11, 0, 0, 86},
    },
    24, 0
};

menu_t<3> midimachinedrum_menu_layout = {
    "MD MIDI",
    {
        //              m  r   n  d  p  f  o
        {"CHRO CHAN:",  0, 18, 2, 9, 0, 0, 18},
        {"POLY CHAN:",  0, 18, 2, 46, 0, 0, 88},
        {"TRIG CHAN:",   0, 18, 2, 57, 0, 0, 18},
    },
    24, 0
};


menu_t<3> mdconfig_menu_layout = {
    "MD",
    {
        //              m  r   n  d  p  f  o
        {"IMPORT",      0, 0,  0, 0, 11, 0, 0},
        {"NORMALIZE:",  0, 2,  2, 8, 0, 0, 16},
        {"POLY CONFIG", 0, 0,  0, 0, 9, 0, 0},
    },
    0, 0
};

menu_t<4> mdimport_menu_layout = {
    "MD",
    {
        //         m  r       n  d  p  f  o
        {"SRC: ",  0, 128, 128, 43, 0, 0, 128},
        {"DEST: ", 0, 128, 128, 44, 0, 0, 128},
        {"COUNT:", 1, 128,  0,  45, 0, 0, 0},
        {"RUN",    0,   0,  0,   0, 0, 25, 0},
    },
    0, 0
};

menu_t<1> mclconfig_menu_layout = {
    "SYSTEM",
    {
        //           m  r  n  d   p  f  o
        {"DISPLAY:", 0, 2, 2, 13, 0, 0, 23},
    },
    1, 0
};

menu_t<7> file_menu_layout = {
    "FILE",
    {
        //            m  r  n  d  p  f  o
        {"CANCEL",    0, 0, 0, 0, 0, 0, 0},
        {"NEW DIR.",  0, 0, 0, 0, 0, 0, 0},
        {"DELETE",    0, 0, 0, 0, 0, 0, 0},
        {"RENAME",    0, 0, 0, 0, 0, 0, 0},
        {"OVERWRITE", 0, 0, 0, 0, 0, 0, 0},
        {"RECV ALL",  0, 0, 0, 0, 0, 0, 0},
        {"SEND ALL",  0, 0, 0, 0, 0, 0, 0},
    },
    0, 0
};

menu_t<20> seq_menu_layout = {
    "SEQ",
    {
        //              m  r                    n                    d   p  f   o
        {"TRACK SEL:",  1, 17,                  0,                   14, 0,  3,  0},
        {"EDIT:",       0, 4,                   4,                   15, 0,  4,  48},
        {"EDIT:",       0, 1 + NUM_LOCKS,       1,                   16, 0,  0,  54},
        {"CC:",         0, 133,                 5,                   17, 0,  0,  55},
        {"SLIDE:",      0, 2,                   2,                   18, 0,  0,  25},
        {"ARPEGGIATOR", 0, 0,                   0,                   0,  10, 0,  0},
        {"TRANSPOSE:",  0, 12,                  0,                   19, 0,  0,  0},
        {"VEL:",        0, 128,                 0,                   20, 0,  0,  0},
        {"COND:",       1, NUM_TRIG_CONDITIONS + 1, NUM_TRIG_CONDITIONS + 1, 21, 0,  0,  60},
        {"SPEED:",      0, 7,                   7,                   22, 0,  5,  41},
        {"LENGTH:",     1, 129,                 0,                   23, 0,  6,  0},
        {"CHANNEL:",    1, 17,                  0,                   24, 0,  7,  0},
        {"COPY:  ",     0, 3,                   3,                   25, 0,  8,  27},
        {"CLEAR:",      0, 3,                   3,                   26, 0,  9,  27},
        {"CLEAR:",      0, 3,                   3,                   26, 0,  10, 30},
        {"PASTE:",      0, 3,                   3,                   27, 0,  11, 27},
        {"SHIFT:",      0, 5,                   5,                   28, 0,  12, 35},
        {"REVERSE:",    0, 3,                   3,                   29, 0,  13, 27},
        {"POLYPHONY",   0, 0,                   0,                   0,  9,  0,  0},
        {"REC QUANT:",  0, 2,                   2,                   42, 0,  0,  25},
    },
    14, 0
};

menu_t<4> step_menu_layout = {
    "STP",
    {
        //             m  r  n  d   p  f   o
        {"CLEAR:",     0, 2, 2, 30, 0, 15, 30},
        {"COPY STEP",  0, 0, 0, 0,  0, 16, 0},
        {"PASTE STEP", 0, 0, 0, 0,  0, 17, 0},
        {"MUTE STEP",  0, 0, 0, 0,  0, 18, 0},
    },
    19, 0
};

menu_t<grid_slot_page_N> slot_menu_layout = {
    "Slot",
    {
        //          m  r    n  d   p  f   o
        {"GRID: ",  0, 2,   2, 31, 0, 0,  52},
        {"MODE:",   1, 4,   3, 32, 0, 0,  20},
        // for non-ext tracks
        {"LEN:   ",   1, 65,  0, 39, 0, 0,  0},
        // for ext tracks
        {"LEN:   ",   1, 129, 0, 39, 0, 0,  0},
        {"LOOP: ",  0, 64,  0, 33, 0, 0,  0},
        // o=128, generate the table on-demand
        {"JUMP: ", 0, 128, 128, 34, 0, 0, 128},
   #ifndef OLED_DISPLAY
        {"APPLY:",  1, 21,  1, 35, 0, 0,  40},
   #endif
        {"CLEAR:",  0, 2,   2, 36, 0, 0,  33},
        {"COPY:  ", 0, 2,   2, 37, 0, 0,  33},
        {"PASTE:",  0, 2,   2, 38, 0, 0,  33},
        {"INSROW:", 0, 127, 1, 50, 0, 0,  33},
        {"RENAME",  0, 0,   0, 0,  0, 20, 0},
    },
    21, 0,
};

menu_t<3> wavdesign_menu_layout = {
    "",
    {
        //           m  r  n  d   p  f   o
        {"EDIT:",    0, 4, 4, 40, 0, 0,  80},
        {"WAV:",     0, 6, 6, 41, 0, 0,  74},
        {"TRANSFER", 0, 0, 0, 0,  0, 22, 0},
    },
    23, 0
};

