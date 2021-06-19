#include "MCL_impl.h"


MCLEncoder param1(GRID_WIDTH - 1, 0, 1);
MCLEncoder param2(GRID_LENGTH - 1, 0 , 1);
MCLEncoder param3(GRID_WIDTH, 1, ENCODER_RES_GRID);
MCLEncoder param4(6, 1, ENCODER_RES_GRID);

GridPage grid_page(&param1, &param2, &param3, &param4);

MCLEncoder gridsave_param1(0, 2, ENCODER_RES_PAT);
MCLEncoder gridload_param1(1, 3, ENCODER_RES_PAT);

MCLEncoder gridio_param2(1, 6, ENCODER_RES_PAT);
MCLEncoder gridio_param3(0, 64, ENCODER_RES_PAT);
MCLEncoder gridio_param4(1, 6, ENCODER_RES_PAT);

GridSavePage grid_save_page(&gridsave_param1, &gridio_param2, &gridio_param3,
                            &gridio_param4);
GridLoadPage grid_load_page(&gridload_param1, &gridio_param2, &gridio_param4,
                             &gridio_param4);

GridTrack slot;

