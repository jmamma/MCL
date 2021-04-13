#include "MCL_impl.h"


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

