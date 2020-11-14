#include "MCLMenus.h"
#include "Project.h"

MCLEncoder options_param1(0, 11, ENCODER_RES_SYS);
MCLEncoder options_param2(0, 17, ENCODER_RES_SYS);

MCLEncoder config_param1(0, 11, ENCODER_RES_SYS);
MCLEncoder config_param2(0, 17, ENCODER_RES_SYS);

MCLEncoder config_param3(0, 17, ENCODER_RES_SYS);
MCLEncoder config_param4(0, 17, ENCODER_RES_SYS);
MCLEncoder config_param5(0, 17, ENCODER_RES_SYS);
MCLEncoder config_param6(0, 17, ENCODER_RES_SYS);
MCLEncoder config_param7(0, 17, ENCODER_RES_SYS);

const menu_option_t MENU_OPTIONS[] PROGMEM = {
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
  // 24: SYSTEM SCREENSAVER
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
};

void new_proj_handler() {
  proj.new_project();
}

const menu_t<8> system_menu_layout PROGMEM = {
    "GLOBAL",
    {
        {"LOAD PROJECT" ,0, 0, 0, (uint8_t *) NULL, (Page*) &load_proj_page, NULL, 0},
        {"NEW PROJECT",0, 0, 0, (uint8_t *) NULL, (Page*) NULL, &new_proj_handler, 0},
        {"MIDI",0, 0, 0, (uint8_t *) NULL, (Page*) &midi_config_page, NULL, 0},
        {"MACHINEDRUM", 0, 0, 0, (uint8_t *) NULL, (Page*) &md_config_page, NULL, 0},
        {"CHAIN MODE", 0, 0, 0, (uint8_t *) NULL, (Page*) &chain_config_page, NULL, 0},
        {"AUX PAGES", 0, 0, 0, (uint8_t *) NULL, (Page*) &aux_config_page, NULL, 0},
        {"SD DRIVE", 0, 0, 0, (uint8_t *) NULL, (Page*) &sddrive_page, NULL, 0},
        {"SYSTEM", 0, 0, 0, (uint8_t *) NULL, (Page*) &mcl_config_page, NULL, 0},
    },
     NULL,
};

const menu_t<1> auxconfig_menu_layout PROGMEM = {
    "AUX PAGES",
    {
        {"RAM Page" ,0, 0, 0, (uint8_t *) NULL, (Page*) &ram_config_page, NULL, 0},
    },
     NULL,
};

const menu_t<1> rampage1_menu_layout PROGMEM = {
    "RAM PAGE",
    {
        {"LINK:", 0, 2, 2, (uint8_t *) &mcl_cfg.ram_page_mode, (Page*) NULL, NULL, 0},
   },
     NULL,
};

const menu_t<5> midiconfig_menu_layout PROGMEM = {
    "MIDI",
    {
        {"TURBO 1:", 0, 4, 4, (uint8_t *) &mcl_cfg.uart1_turbo, (Page*) NULL, NULL, 2},
        {"TURBO 2:", 0, 4, 4, (uint8_t *) &mcl_cfg.uart2_turbo, (Page*) NULL, NULL, 2},

        {"CLK REC:", 0, 2, 2, (uint8_t *) &mcl_cfg.clock_rec, (Page*) NULL, NULL, 6},
        {"CLK SEND:", 0,  2, 2, (uint8_t *) &mcl_cfg.clock_send, (Page*) NULL, NULL, 8},

        {"MIDI FWD:", 0, 3, 3, (uint8_t *) &mcl_cfg.midi_forward, (Page*) NULL, NULL, 10},
   },

    (&mclsys_apply_config),
};

const menu_t<4> mdconfig_menu_layout PROGMEM = {
    "MD",
    {
        {"TRACK SELECT:",0, 2, 2, (uint8_t *) &mcl_cfg.track_select, (Page*) NULL, NULL, 13},
        {"NORMALIZE:",0, 2, 2, (uint8_t *) &mcl_cfg.auto_normalize, (Page*) NULL, NULL, 15},
        {"CTRL CHAN:",0, 18, 2, (uint8_t *) &mcl_cfg.uart2_ctrl_mode, (Page*) NULL, NULL, 17},
        {"POLY CONFIG", 0, 0, 0, (uint8_t *) NULL, (Page*) &poly_page, NULL, 0},
    },
    (&mclsys_apply_config),
};

const menu_t<3> chain_menu_layout PROGMEM = {
    "CHAIN",
    {
        {"CHAIN:", 1, 4, 3, (uint8_t *) &mcl_cfg.chain_mode, (Page*) NULL, NULL, 19},
        {"RAND MIN:", 0, 128, 0, (uint8_t *) &mcl_cfg.chain_rand_min, (Page*) NULL, NULL, 0},
        {"RAND MAX:", 0, 128, 0, (uint8_t *) &mcl_cfg.chain_rand_max, (Page*) NULL, NULL, 0},
    },
    (&mclsys_apply_config),
};


const menu_t<2> mclconfig_menu_layout PROGMEM = {
    "SYSTEM",
    {
        {"DISPLAY:", 0, 2, 2, (uint8_t *) &mcl_cfg.display_mirror, (Page*) NULL, NULL, 22},
        {"SCREENSAVER:", 0, 2, 2, (uint8_t *) &mcl_cfg.screen_saver, (Page*) NULL, NULL, 24},
        //{"DIAGNOSTIC:", 0, 0, 0, (uint8_t *) NULL, (Page*) &diag_page, NULL, {}},
    },
    (&mclsys_apply_config),
};

const menu_t<5> file_menu_layout PROGMEM = {
    "FILE",
    {
        {"NEW DIR.", 0, 0, 0, (uint8_t *)NULL, (Page *)NULL, NULL, 0},
        {"DELETE", 0, 0, 0, (uint8_t *)NULL, (Page *)NULL, NULL, 0},
        {"RENAME", 0, 0, 0, (uint8_t *)NULL, (Page *)NULL, NULL, 0},
        {"OVERWRITE", 0, 0, 0, (uint8_t *)NULL, (Page *)NULL, NULL, 0},
        {"CANCEL", 0, 0, 0, (uint8_t *)NULL, (Page *)NULL, NULL, 0},
    },
    NULL,
};

MenuPage<1> aux_config_page(&auxconfig_menu_layout, &config_param1, &config_param6);
MenuPage<8> system_page(&system_menu_layout, &options_param1, &options_param2);
MenuPage<5> midi_config_page(&midiconfig_menu_layout, &config_param1,
                          &config_param3);
MenuPage<4> md_config_page(&mdconfig_menu_layout, &config_param1, &config_param4);
MenuPage<3> chain_config_page(&chain_menu_layout, &config_param1, &config_param6);
MenuPage<2> mcl_config_page(&mclconfig_menu_layout, &config_param1,
                         &config_param5);
MenuPage<1> ram_config_page(&rampage1_menu_layout, &config_param1,
                         &config_param7);


MCLEncoder input_encoder1(0, 127, ENCODER_RES_SYS);
MCLEncoder input_encoder2(0, 127, ENCODER_RES_SYS);

TextInputPage text_input_page(&input_encoder1, &input_encoder2);

MCLEncoder file_menu_encoder(0, 4, ENCODER_RES_PAT);
MenuPage<5> file_menu_page(&file_menu_layout, &config_param1, &file_menu_encoder);

