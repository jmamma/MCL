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
menu_t<7> system_menu_layout = {
    "GLOBAL",
    {
        //               m  r  n  d  p  f  o
        {"LOAD PROJECT", 0, 0, 0, 0, 1, 0, 0},
        {"CONV PROJECT", 0, 0, 0, 0, 2, 0, 0},
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

menu_t<6> midiconfig_menu_layout = {
    "MIDI",
    {
        //            m  r  n  d  p  f  o
        {"TURBO 1:",  0, 4, 4, 2, 0, 0, 2},
        {"TURBO 2:",  0, 4, 4, 3, 0, 0, 2},
        {"DEVICE 2:", 0, 2, 2, 4, 0, 0, 82},

        {"CLK REC:",  0, 2, 2, 5, 0, 0, 6},
        {"CLK SEND:", 0, 2, 2, 6, 0, 0, 8},

        {"MIDI FWD:", 0, 4, 4, 7, 0, 0, 10},
    },
    24, 0
};

menu_t<3> mdconfig_menu_layout = {
    "MD",
    {
        //              m  r   n  d  p  f  o
        {"NORMALIZE:",  0, 2,  2, 8, 0, 0, 16},
        {"CTRL CHAN:",  0, 18, 2, 9, 0, 0, 18},
        {"POLY CONFIG", 0, 0,  0, 0, 9, 0, 0},
    },
    1, 0
};

menu_t<1> mclconfig_menu_layout = {
    "SYSTEM",
    {
        //           m  r  n  d   p  f  o
        {"DISPLAY:", 0, 2, 2, 13, 0, 0, 23},
        //{"DIAGNOSTIC:", 0, 0, 0, (uint8_t *) NULL, nullptr, NULL, {}},
    },
    1, 0
};

menu_t<5> file_menu_layout = {
    "FILE",
    {
        //            m  r  n  d  p  f  o
        {"NEW DIR.",  0, 0, 0, 0, 0, 0, 0},
        {"DELETE",    0, 0, 0, 0, 0, 0, 0},
        {"RENAME",    0, 0, 0, 0, 0, 0, 0},
        {"OVERWRITE", 0, 0, 0, 0, 0, 0, 0},
        {"CANCEL",    0, 0, 0, 0, 0, 0, 0},
    },
    0, 0
};

menu_t<19> seq_menu_layout = {
    "SEQ",
    {
        //              m  r                    n                    d   p  f   o
        {"TRACK SEL:",  1, 17,                  0,                   14, 0,  3,  0},
        {"EDIT:",       0, 4,                   4,                   15, 0,  4,  48},
        {"EDIT:",       0, 1 + NUM_LOCKS,       1,                   16, 0,  0,  54},
        {"CC:",         0, 131,                 3,                   17, 0,  0,  55},
        {"SLIDE:",      0, 2,                   2,                   18, 0,  0,  25},
        {"ARPEGGIATOR", 0, 0,                   0,                   0,  10, 0,  0},
        {"TRANSPOSE:",  0, 12,                  0,                   19, 0,  0,  0},
        {"VEL:",        0, 128,                 0,                   20, 0,  0,  0},
        {"COND:",       1, NUM_TRIG_CONDITIONS + 1, NUM_TRIG_CONDITIONS + 1, 21, 0,  0,  58},
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
        {"RENAME",  0, 0,   0, 0,  0, 20, 0},
    },
    21, 0,
};

menu_t<3> wav_menu_layout = {
    "",
    {
        //           m  r  n  d   p  f   o
        {"EDIT:",    0, 4, 4, 40, 0, 0,  78},
        {"WAV:",     0, 6, 6, 41, 0, 0,  72},
        {"TRANSFER", 0, 0, 0, 0,  0, 22, 0},
    },
    23, 0
};

