#include "GridPages.h"
#include "MCL.h"

#ifdef OLED_DISPLAY
MCLEncoder param1(GRID_WIDTH - 1, 0, 1);
#else
MCLEncoder param1(GRID_WIDTH - 4, 0, 1);
#endif
MCLEncoder param2(GRID_LENGTH - 1, 0 , 1);
MCLEncoder param3(0, 127, 1);
MCLEncoder param4(0, 127, 1);

GridPage grid_page(&param1, &param2, &param3, &param4);

MCLEncoder gridio_param1(0, 8, ENCODER_RES_PAT);
MCLEncoder gridio_param2(0, 15, ENCODER_RES_PAT);
MCLEncoder gridio_param3(0, 64, ENCODER_RES_PAT);
MCLEncoder gridio_param4(0, 11, ENCODER_RES_PAT);

GridSavePage grid_save_page(&gridio_param1, &gridio_param2, &gridio_param3,
                            &gridio_param4);
GridWritePage grid_write_page(&gridio_param1, &gridio_param2, &gridio_param3,
                             &gridio_param4);

GridTrack slot;

MCLEncoder grid_slot_param1(0, 8, ENCODER_RES_PAT);
MCLEncoder grid_slot_param2(0, 8, ENCODER_RES_PAT);
MenuPage grid_slot_page(&slot_menu_layout, &grid_slot_param1, &grid_slot_param2);

