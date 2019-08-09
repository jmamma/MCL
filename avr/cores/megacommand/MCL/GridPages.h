/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef GRIDPAGES_H__
#define GRIDPAGES_H__

#ifdef OLED_DISPLAY

#define ENCODER_RES_GRID 1
#define ENCODER_RES_PAT 2

#else

#define ENCODER_RES_GRID 4
#define ENCODER_RES_PAT 4

#endif

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
    #ifndef OLED_DISPLAY
    8,
    #else
    7,
    #endif
    {
        {"CHAIN:", 1, 4, 3, (uint8_t *) &mcl_cfg.chain_mode, (Page*) NULL, (void*)NULL, {{1, "AUT"},{2,"MAN"},{3,"RND"}}},
        {"LOOP:  ", 0, 64, 0, (uint8_t *) &slot.chain.loops,  (Page*) NULL, (void*)NULL, {}},
        {"ROW:    ", 0, 128, 0, (uint8_t*) &slot.chain.row, (Page*) NULL, (void*)NULL, {}},
   #ifndef OLED_DISPLAY
        {"APPLY:", 1, 21, 1, (uint8_t *) &grid_page.slot_apply, (Page*) NULL, (void*)NULL, {{0," "}}},
   #endif
        {"CLEAR:", 0, 2, 2, (uint8_t *) &grid_page.slot_clear, (Page*) NULL, (void*)NULL, {{0,"--"},{1, "YES"}}},
        {"COPY:", 0, 2, 2, (uint8_t *) &grid_page.slot_copy, (Page*) NULL, (void*)NULL, {{0,"--"},{1, "YES"}}},
        {"PASTE:", 0, 2, 2, (uint8_t *) &grid_page.slot_paste, (Page*) NULL, (void*)NULL,{{0,"--"},{1, "YES"}}},
        {"RENAME", 0, 0, 0, (uint8_t *) NULL, (Page*) NULL, (void*)&rename_row, {}},
    },
    (void*)&apply_slot_changes_cb,
    (Page*)NULL,
};

extern MCLEncoder grid_slot_param1;
extern MCLEncoder grid_slot_param2;

extern MenuPage grid_slot_page;
#endif /* GRIDPAGES_H__ */
