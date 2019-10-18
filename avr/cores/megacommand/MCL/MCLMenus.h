/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MCLMENUS_H__
#define MCLMENUS_H__

#include "MCLEncoder.h"
#include "MCLSysConfig.h"
#include "ProjectPages.h"
#include "PolyPage.h"
#include "SDDrivePage.h"
#include "GridPages.h"
#include "TextInputPage.h"
#include "SeqPages.h"

#define ENCODER_RES_SYS 2

extern MCLEncoder options_param1;
extern MCLEncoder options_param2;
extern MCLEncoder config_param1;
extern MCLEncoder config_param2;

extern MCLEncoder config_param3;
extern MCLEncoder config_param4;
extern MCLEncoder config_param5;
extern MCLEncoder config_param6;
extern MCLEncoder config_param7;

extern MenuPage system_page;
extern MenuPage midi_config_page;
extern MenuPage md_config_page;
extern MenuPage mcl_config_page;
extern MenuPage chain_config_page;
extern MenuPage aux_config_page;
extern MenuPage ram_config_page;

extern MCLEncoder input_encoder1;
extern MCLEncoder input_encoder2;

extern TextInputPage text_input_page;

const menu_t system_menu_layout PROGMEM = {
    "GLOBAL",
    7,
    {
        {"LOAD PROJECT" ,0, 0, 0, (uint8_t *) NULL, (Page*) &load_proj_page, (void*)NULL, {}},
        {"NEW PROJECT",0, 0, 0, (uint8_t *) NULL, (Page*) &new_proj_page, (void*)NULL, {}},
        {"MIDI",0, 0, 0, (uint8_t *) NULL, (Page*) &midi_config_page, (void*)NULL, {}},
        {"MACHINEDRUM", 0, 0, 0, (uint8_t *) NULL, (Page*) &md_config_page, (void*)NULL, {}},
        {"CHAIN MODE", 0, 0, 0, (uint8_t *) NULL, (Page*) &chain_config_page, (void*)NULL, {}},
        {"AUX PAGES", 0, 0, 0, (uint8_t *) NULL, (Page*) &aux_config_page, (void*)NULL, {}},
        {"SYSTEM", 0, 0, 0, (uint8_t *) NULL, (Page*) &mcl_config_page, (void*)NULL, {}},
    },
    (void*) NULL,
};

const menu_t auxconfig_menu_layout PROGMEM = {
    "AUX PAGES",
    1,
    {
        {"RAM Page" ,0, 0, 0, (uint8_t *) NULL, (Page*) &ram_config_page, (void*)NULL, {}},
    },
    (void*) NULL,
};

const menu_t rampage1_menu_layout PROGMEM = {
    "RAM PAGE",
    1,
    {
        {"LINK:", 0, 2, 2, (uint8_t *) &mcl_cfg.ram_page_mode, (Page*) NULL, (void*)NULL, {{0, "MONO"},{1, "STEREO"}}},
   },

    (void*) NULL,

};


const menu_t midiconfig_menu_layout PROGMEM = {
    "MIDI",
    5,
    {
        {"TURBO 1:", 0, 4, 4, (uint8_t *) &mcl_cfg.uart1_turbo, (Page*) NULL, (void*)NULL, {{0, "1x"},{1, "2x"},{2,"4x"},{3,"8x"}}},
        {"TURBO 2:", 0, 4, 4, (uint8_t *) &mcl_cfg.uart2_turbo, (Page*) NULL, (void*)NULL, {{0, "1x"},{1, "2x"},{2,"4x"},{3,"8x"}}},

        {"CLK REC:", 0, 2, 2, (uint8_t *) &mcl_cfg.clock_rec, (Page*) NULL, (void*)NULL, {{0, "MIDI 1"},{1, "MIDI 2"}}},
        {"CLK SEND:", 0,  2, 2, (uint8_t *) &mcl_cfg.clock_send, (Page*) NULL, (void*)NULL, {{0, "OFF"},{1, "MIDI 2"}}},

        {"MIDI FWD:", 0, 3, 3, (uint8_t *) &mcl_cfg.midi_forward, (Page*) NULL, (void*)NULL, {{0, "OFF"}, {1, "1->2"},{2, "2->1"}}},
   },

    (void*)(&mclsys_apply_config),

};

const menu_t mdconfig_menu_layout PROGMEM = {
    "MD",
    5,
    {
        {"KIT SAVE:",0, 2, 2, (uint8_t *) &mcl_cfg.auto_save, (Page*) NULL, (void*)NULL, {{0, "OFF"},{1, "AUTO"}}},
        {"NORMALIZE:",0, 2, 2, (uint8_t *) &mcl_cfg.auto_normalize, (Page*) NULL, (void*)NULL, {{0, "OFF"},{1, "AUTO"}}},
        {"CTRL CHAN:",0, 18, 2, (uint8_t *) &mcl_cfg.uart2_ctrl_mode, (Page*) NULL, (void*)NULL, {{0, "INT"},{17, "OMNI"}}},
        {"POLY CONFIG", 0, 0, 0, (uint8_t *) NULL, (Page*) &poly_page, (void*)NULL, {}},
        {"SD DRIVE", 0, 0, 0, (uint8_t *) NULL, (Page*) &sddrive_page, (void*)NULL, {}},
    },
    (void*)(&mclsys_apply_config),
};

const menu_t chain_menu_layout PROGMEM = {
    "CHAIN",
    3,
    {
        {"CHAIN:", 1, 4, 3, (uint8_t *) &mcl_cfg.chain_mode, (Page*) NULL, (void*)NULL, {{1, "AUT"},{2,"MAN"},{3,"RND"}}},
        {"RAND MIN:", 0, 128, 0, (uint8_t *) &mcl_cfg.chain_rand_min, (Page*) NULL, (void*)NULL, {}},
        {"RAND MAX:", 0, 128, 0, (uint8_t *) &mcl_cfg.chain_rand_max, (Page*) NULL, (void*)NULL, {}},
    },
    (void*)(&mclsys_apply_config),
};


const menu_t mclconfig_menu_layout PROGMEM = {
    "SYSTEM",
    2,
    {
        {"DISPLAY:", 0, 2, 2, (uint8_t *) &mcl_cfg.display_mirror, (Page*) NULL, (void*)NULL, {{0, "INT"}, {1, "INT+EXT"}}},
        {"SCREENSAVER:", 0, 2, 2, (uint8_t *) &mcl_cfg.screen_saver, (Page*) NULL, (void*)NULL, {{0, "OFF"}, {1, "ON"}}},
    },
    (void*)(&mclsys_apply_config),
};



#endif /* MCLMENUS_H__ */
