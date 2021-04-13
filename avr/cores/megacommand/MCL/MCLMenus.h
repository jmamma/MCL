/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MCLMENUS_H__
#define MCLMENUS_H__

#include "MCLEncoder.h"
#include "MCLSysConfig.h"
#include "ProjectPages.h"
#include "PolyPage.h"
#include "SDDrivePage.h"
#include "GridPages.h"
#include "TextInputPage.h"
#include "SeqPages.h"
#include "DiagnosticPage.h"
#define ENCODER_RES_SYS 2

extern MCLEncoder options_param1;
extern MCLEncoder options_param2;
extern MCLEncoder config_param1;
extern MCLEncoder config_param2;

extern MCLEncoder config_param3;
extern MCLEncoder config_param4;
extern MCLEncoder config_param5;
extern MCLEncoder config_param6;
extern MCLEncoder config_param7;

extern void new_proj_handler();

extern MenuPage<9> system_page;
extern MenuPage<6> midi_config_page;
extern MenuPage<3> md_config_page;
extern MenuPage<1> mcl_config_page;
extern MenuPage<3> chain_config_page;
extern MenuPage<1> aux_config_page;
extern MenuPage<1> ram_config_page;

extern MCLEncoder input_encoder1;
extern MCLEncoder input_encoder2;

extern TextInputPage text_input_page;

extern MCLEncoder file_menu_encoder;
extern MenuPage<5> file_menu_page;


#endif /* MCLMENUS_H__ */
