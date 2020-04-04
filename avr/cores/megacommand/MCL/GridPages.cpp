#include "GridPages.h"
#include "MCL.h"

const menu_t<
    #ifndef OLED_DISPLAY
    8
    #else
    7
    #endif
    >
slot_menu_layout PROGMEM = {
    "Slot",
    {
        {"CHAIN:", 1, 4, 3, (uint8_t *) &mcl_cfg.chain_mode, (Page*) NULL, NULL, 19},
        {"LOOP:  ", 0, 64, 0, (uint8_t *) &slot.chain.loops,  (Page*) NULL, NULL, 0},
        {"ROW:    ", 0, 128, 0, (uint8_t*) &slot.chain.row, (Page*) NULL, NULL, 0},
   #ifndef OLED_DISPLAY
        {"APPLY:", 1, 21, 1, (uint8_t *) &grid_page.slot_apply, (Page*) NULL, NULL, 39},
   #endif
        {"CLEAR:", 0, 2, 2, (uint8_t *) &grid_page.slot_clear, (Page*) NULL, NULL, 40},
        {"COPY:", 0, 2, 2, (uint8_t *) &grid_page.slot_copy, (Page*) NULL, NULL, 40},
        {"PASTE:", 0, 2, 2, (uint8_t *) &grid_page.slot_paste, (Page*) NULL, NULL, 40},
        {"RENAME", 0, 0, 0, (uint8_t *) NULL, (Page*) NULL, &rename_row, 0},
    },
    &apply_slot_changes_cb,
    (Page*)NULL,
};


#ifdef OLED_DISPLAY
MCLEncoder param1(GRID_WIDTH - 1, 0, 1);
MCLEncoder param2(GRID_LENGTH - 1, 0 , 1);
#else
MCLEncoder param1(GRID_WIDTH - 4, 0, ENCODER_RES_GRID);
MCLEncoder param2(GRID_LENGTH - 1, 0 , ENCODER_RES_GRID);
#endif

MCLEncoder param3(GRID_WIDTH, 1, ENCODER_RES_GRID);
MCLEncoder param4(GRID_LENGTH, 1, ENCODER_RES_GRID);

GridPage grid_page(&param1, &param2, &param3, &param4);

MCLEncoder gridio_param1(0, 2, ENCODER_RES_PAT);
MCLEncoder gridio_param2(0, 15, ENCODER_RES_PAT);
MCLEncoder gridio_param3(0, 64, ENCODER_RES_PAT);
MCLEncoder gridio_param4(1, 11, ENCODER_RES_PAT);

GridSavePage grid_save_page(&gridio_param1, &gridio_param2, &gridio_param3,
                            &gridio_param4);
GridWritePage grid_write_page(&gridio_param1, &gridio_param2, &gridio_param3,
                             &gridio_param4);

GridTrack slot;

MCLEncoder grid_slot_param1(0, 7, ENCODER_RES_PAT);
MCLEncoder grid_slot_param2(0, 16, ENCODER_RES_PAT);
MenuPage<
    #ifndef OLED_DISPLAY
    8
    #else
    7
    #endif
>
grid_slot_page(&slot_menu_layout, &grid_slot_param1, &grid_slot_param2);

