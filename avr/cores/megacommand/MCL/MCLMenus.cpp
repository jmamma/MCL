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

void new_proj_handler() {
  proj.new_project();
}

const menu_t<7> system_menu_layout PROGMEM = {
    "GLOBAL",
    {
        {"LOAD PROJECT" ,0, 0, 0, (uint8_t *) NULL, (Page*) &load_proj_page, NULL, {}},
        {"NEW PROJECT",0, 0, 0, (uint8_t *) NULL, (Page*) NULL, &new_proj_handler, {}},
        {"MIDI",0, 0, 0, (uint8_t *) NULL, (Page*) &midi_config_page, NULL, {}},
        {"MACHINEDRUM", 0, 0, 0, (uint8_t *) NULL, (Page*) &md_config_page, NULL, {}},
        {"CHAIN MODE", 0, 0, 0, (uint8_t *) NULL, (Page*) &chain_config_page, NULL, {}},
        {"AUX PAGES", 0, 0, 0, (uint8_t *) NULL, (Page*) &aux_config_page, NULL, {}},
        {"SYSTEM", 0, 0, 0, (uint8_t *) NULL, (Page*) &mcl_config_page, NULL, {}},
    },
     NULL,
};

const menu_t<1> auxconfig_menu_layout PROGMEM = {
    "AUX PAGES",
    {
        {"RAM Page" ,0, 0, 0, (uint8_t *) NULL, (Page*) &ram_config_page, NULL, {}},
    },
     NULL,
};

const menu_t<1> rampage1_menu_layout PROGMEM = {
    "RAM PAGE",
    {
        {"LINK:", 0, 2, 2, (uint8_t *) &mcl_cfg.ram_page_mode, (Page*) NULL, NULL, {{0, "MONO"},{1, "STEREO"}}},
   },
     NULL,
};

const menu_t<5> midiconfig_menu_layout PROGMEM = {
    "MIDI",
    {
        {"TURBO 1:", 0, 4, 4, (uint8_t *) &mcl_cfg.uart1_turbo, (Page*) NULL, NULL, {{0, "1x"},{1, "2x"},{2,"4x"},{3,"8x"}}},
        {"TURBO 2:", 0, 4, 4, (uint8_t *) &mcl_cfg.uart2_turbo, (Page*) NULL, NULL, {{0, "1x"},{1, "2x"},{2,"4x"},{3,"8x"}}},

        {"CLK REC:", 0, 2, 2, (uint8_t *) &mcl_cfg.clock_rec, (Page*) NULL, NULL, {{0, "MIDI 1"},{1, "MIDI 2"}}},
        {"CLK SEND:", 0,  2, 2, (uint8_t *) &mcl_cfg.clock_send, (Page*) NULL, NULL, {{0, "OFF"},{1, "MIDI 2"}}},

        {"MIDI FWD:", 0, 3, 3, (uint8_t *) &mcl_cfg.midi_forward, (Page*) NULL, NULL, {{0, "OFF"}, {1, "1->2"},{2, "2->1"}}},
   },

    (&mclsys_apply_config),
};

const menu_t<5> mdconfig_menu_layout PROGMEM = {
    "MD",
    {
        {"TRACK SELECT:",0, 2, 2, (uint8_t *) &mcl_cfg.track_select, (Page*) NULL, NULL, {{0, "MAN"},{1, "AUTO"}}},
        {"NORMALIZE:",0, 2, 2, (uint8_t *) &mcl_cfg.auto_normalize, (Page*) NULL, NULL, {{0, "OFF"},{1, "AUTO"}}},
        {"CTRL CHAN:",0, 18, 2, (uint8_t *) &mcl_cfg.uart2_ctrl_mode, (Page*) NULL, NULL, {{0, "INT"},{17, "OMNI"}}},
        {"POLY CONFIG", 0, 0, 0, (uint8_t *) NULL, (Page*) &poly_page, NULL, {}},
        {"SD DRIVE", 0, 0, 0, (uint8_t *) NULL, (Page*) &sddrive_page, NULL, {}},
    },
    (&mclsys_apply_config),
};

const menu_t<3> chain_menu_layout PROGMEM = {
    "CHAIN",
    {
        {"CHAIN:", 1, 4, 3, (uint8_t *) &mcl_cfg.chain_mode, (Page*) NULL, NULL, {{1, "AUT"},{2,"MAN"},{3,"RND"}}},
        {"RAND MIN:", 0, 128, 0, (uint8_t *) &mcl_cfg.chain_rand_min, (Page*) NULL, NULL, {}},
        {"RAND MAX:", 0, 128, 0, (uint8_t *) &mcl_cfg.chain_rand_max, (Page*) NULL, NULL, {}},
    },
    (&mclsys_apply_config),
};


const menu_t<2> mclconfig_menu_layout PROGMEM = {
    "SYSTEM",
    {
        {"DISPLAY:", 0, 2, 2, (uint8_t *) &mcl_cfg.display_mirror, (Page*) NULL, NULL, {{0, "INT"}, {1, "INT+EXT"}}},
        {"SCREENSAVER:", 0, 2, 2, (uint8_t *) &mcl_cfg.screen_saver, (Page*) NULL, NULL, {{0, "OFF"}, {1, "ON"}}},
        //{"DIAGNOSTIC:", 0, 0, 0, (uint8_t *) NULL, (Page*) &diag_page, NULL, {}},
    },
    (&mclsys_apply_config),
};

const menu_t<5> file_menu_layout PROGMEM = {
    "FILE",
    {
        {"NEW DIR.", 0, 0, 0, (uint8_t *)NULL, (Page *)NULL, NULL, {}},
        {"DELETE", 0, 0, 0, (uint8_t *)NULL, (Page *)NULL, NULL, {}},
        {"RENAME", 0, 0, 0, (uint8_t *)NULL, (Page *)NULL, NULL, {}},
        {"OVERWRITE", 0, 0, 0, (uint8_t *)NULL, (Page *)NULL, NULL, {}},
        {"CANCEL", 0, 0, 0, (uint8_t *)NULL, (Page *)NULL, NULL, {}},
    },
    NULL,
};

MenuPage<1> aux_config_page(&auxconfig_menu_layout, &config_param1, &config_param6);
MenuPage<7> system_page(&system_menu_layout, &options_param1, &options_param2);
MenuPage<5> midi_config_page(&midiconfig_menu_layout, &config_param1,
                          &config_param3);
MenuPage<5> md_config_page(&mdconfig_menu_layout, &config_param1, &config_param4);
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

