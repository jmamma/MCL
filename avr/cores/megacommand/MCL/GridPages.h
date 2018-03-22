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


extern GridEncoder param1;
extern GridEncoder param2;
extern GridEncoder param3;
extern GridEncoder param4;

extern GridPage grid_page;

extern MCLEncoder gridio_param1;
extern MCLEncoder gridio_param2;
extern MCLEncoder gridio_param3;
extern MCLEncoder gridio_param4;

extern GridSavePage grid_save_page;
extern GridWritePage grid_write_page;

#endif /* GRIDPAGES_H__ */
