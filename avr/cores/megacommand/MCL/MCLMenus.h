/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MCLMENUS_H__
#define MCLMENUS_H__

#include "MCLEncoder.h"
#include "MCLSysConfig.h"
#include "ProjectPages.h"
#include "GridPages.h"

#define ENCODER_RES_SYS 2

extern MCLEncoder options_param1;
extern MCLEncoder options_param2;
extern MCLEncoder config_param1;
extern MCLEncoder config_param2;

extern MenuPage system_page;
extern MenuPage midi_config_page;
extern MenuPage md_config_page;

const menu_t system_menu_layout PROGMEM = {
    "SYSTEM ",
    5,
    {
        {"LOAD PROJECT" ,0, 0, 0, (uint8_t *) NULL, (Page*) &load_proj_page, {}},
        {"NEW PROJECT",0, 0, 0, (uint8_t *) NULL, (Page*) &new_proj_page, {}},
        {"MIDI CONFIG",0, 0, 0, (uint8_t *) NULL, (Page*) &midi_config_page, {}},
        {"MD CONFIG", 0, 0, 0, (uint8_t *) NULL, (Page*) &md_config_page, {}},
        {"DISPLAY:", 0, 2, 2, (uint8_t *) &mcl_cfg.display_mirror, (Page*) NULL, {{0, "INT"}, {1, "INT+EXT"}}},
    },
    (void*)(&mclsys_apply_config),
};

const menu_t midiconfig_menu_layout PROGMEM = {
    "MIDI",
    5,
    {
        {"TURBO 1:", 0, 4, 4, (uint8_t *) &mcl_cfg.uart1_turbo, (Page*) NULL, {{0, "1x"},{1, "2x"},{2,"4x"},{3,"8x"}}},
        {"TURBO 2:", 0, 4, 4, (uint8_t *) &mcl_cfg.uart2_turbo, (Page*) NULL, {{0, "1x"},{1, "2x"},{2,"4x"},{3,"8x"}}},

        {"CLK REC:", 0, 2, 2, (uint8_t *) &mcl_cfg.clock_rec, (Page*) NULL, {{0, "MIDI 1"},{1, "MIDI 2"}}},
        {"CLK SEND:", 0,  2, 2, (uint8_t *) &mcl_cfg.clock_send, (Page*) NULL, {{0, "OFF"},{1, "MIDI 2"}}},

        {"MIDI FWD:", 0, 3, 3, (uint8_t *) &mcl_cfg.midi_forward, (Page*) NULL, {{0, "OFF"}, {1, "1->2"},{2, "2->1"}}},
   },
    (void*) NULL,

};

const menu_t mdconfig_menu_layout PROGMEM = {
    "MD",
    4,
    {
        {"MD SAVE:",0, 2, 2, (uint8_t *) &mcl_cfg.auto_save, (Page*) NULL, {{0, "OFF"},{1, "AUTO"}}},
        {"MD POLY-START:",0, 17, 0, (uint8_t *) &mcl_cfg.poly_start, (Page*) NULL, {}},
        {"MD POLY-MAX:", 1, 17, 0, (uint8_t *) &mcl_cfg.poly_max, (Page*) NULL, {}},
        {"MD CTRL-CHAN:",1, 19, 2, (uint8_t *) &mcl_cfg.uart2_ctrl_mode, (Page*) NULL, {{17, "INT"},{18, "OMNI"}}},

    },
    (void*) NULL,

};

#endif /* MCLMENUS_H__ */
