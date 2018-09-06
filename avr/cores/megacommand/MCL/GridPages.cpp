#include "GridPages.h"
#include "MCL.h"

GridEncoder param1(GRID_WIDTH - 1, 0, 1);
GridEncoder param2(GRID_LENGTH - 1, 0 , 1);
GridEncoder param3(0, 127, 1);
GridEncoder param4(0, 127, 1);

GridPage grid_page(&param1, &param2, &param3, &param4);

MCLEncoder gridio_param1(0, 8, ENCODER_RES_PAT);
MCLEncoder gridio_param2(0, 15, ENCODER_RES_PAT);
MCLEncoder gridio_param3(0, 64, ENCODER_RES_PAT);
MCLEncoder gridio_param4(0, 11, ENCODER_RES_PAT);

GridSavePage grid_save_page(&gridio_param1, &gridio_param2, &gridio_param3,
                            &gridio_param4);
GridWritePage grid_write_page(&gridio_param1, &gridio_param2, &gridio_param3,
                             &gridio_param4);

