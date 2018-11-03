/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef GRIDPAGES_H__
#define GRIDPAGES_H__

#define ENCODER_RES_GRID 2
#define ENCODER_RES_PAT 2

#include "GridEncoder.h"
#include "GridPage.h"

#include "MCLEncoder.h"
#include "GridSavePage.h"
#include "GridWritePage.h"
#include "Menu.h"
#include "MenuPage.h"
#include "GridChain.h"
#include "GridTrack.h"
#include "MCLSysConfig.h"

extern MCLEncoder param1;
extern MCLEncoder param2;
extern MCLEncoder param3;
extern MCLEncoder param4;

extern GridPage grid_page;

extern MCLEncoder gridio_param1;
extern MCLEncoder gridio_param2;
extern MCLEncoder gridio_param3;
extern MCLEncoder gridio_param4;

extern GridSavePage grid_save_page;
extern GridWritePage grid_write_page;


extern GridTrack slot;

const menu_t slot_menu_layout PROGMEM = {
    "Slot",
    5,
    {
        {"CHAIN:", 0, 4, 4, (uint8_t *) &mcl_cfg.chain_mode, (Page*) NULL, {{0, "OFF"},{1, "AUT"},{2,"MAN"},{3,"RND"}}},
        {"LOOP:  ", 0, 128, 0, (uint8_t *) &slot.chain.loops,  (Page*) NULL, {}},
        {"ROW:    ", 0, 128 - 1, 0, (uint8_t*) &slot.chain.row, NULL, {}},
        {"APPLY:", 1, 21, 1, (uint8_t *) &grid_page.slot_apply, (Page*) NULL, {{0," "}}},
        {"MERGE", 0, 2, 2, (uint8_t *) &grid_page.merge_md, (Page*) NULL, {{0, "--"},{1, "SEQ"}}}
    },
    (void*)NULL,
    (Page*)NULL,
};

extern MCLEncoder grid_slot_param1;
extern MCLEncoder grid_slot_param2;

extern MenuPage grid_slot_page;
#endif /* GRIDPAGES_H__ */
