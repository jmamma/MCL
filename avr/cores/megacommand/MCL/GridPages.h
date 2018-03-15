/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef GRIDPAGES_H__
#define GRIDPAGES_H__


extern GridEncoder param1(0, GRID_WIDTH - 4, ENCODER_RES_GRID);
extern GridEncoder param2(0, 127, ENCODER_RES_GRID);
extern GridEncoder param3(0, 127, 1);
extern GridEncoder param4(0, 127, 1);

extern GridPage grid_page(&param1, &param2, &param3, &param4);

extern MCLEncoder gridio_param1(0, 8, ENCODER_RES_PAT);
extern MCLEncoder gridio_param2(0, 15, ENCODER_RES_PAT);
extern MCLEncoder gridio_param3(0, 64, ENCODER_RES_PAT);
extern MCLEncoder gridio_param4(0, 11, ENCODER_RES_PAT);

extern GridSavePage grid_save_page(&gridio_param1, &gridio_param2, &gridio_param3, &gridio_param4);
extern GridWritePage grid_write_pag(&gridio_param1, &gridio_param2, &gridio_param3, &gridio_param4);

extern Grid grid;
#endif /* GRIDPAGES_H__ */
