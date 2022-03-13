/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MCLMENUS_H__
#define MCLMENUS_H__

#include "MCLEncoder.h"
#include "MCLSysConfig.h"
#include "ProjectPages.h"
#include "PolyPage.h"
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
extern MCLEncoder config_param8;

extern void new_proj_handler();

extern MenuPage<2> start_menu_page;
extern MenuPage<6> system_page;
extern MenuPage<10> midi_config_page;
extern MenuPage<5> md_config_page;
extern MenuPage<1> mcl_config_page;
extern MenuPage<3> chain_config_page;
extern MenuPage<1> aux_config_page;
extern MenuPage<1> ram_config_page;
extern MenuPage<4> md_import_page;

extern MCLEncoder input_encoder1;
extern MCLEncoder input_encoder2;

extern TextInputPage text_input_page;

extern MCLEncoder file_menu_encoder;
extern MenuPage<7> file_menu_page;

extern MCLEncoder seq_menu_value_encoder;
extern MCLEncoder seq_menu_entry_encoder;
extern MenuPage<20> seq_menu_page;

extern MCLEncoder step_menu_value_encoder;
extern MCLEncoder step_menu_entry_encoder;
extern MenuPage<4> step_menu_page;

extern MCLEncoder grid_slot_param1;
extern MCLEncoder grid_slot_param2;

constexpr size_t grid_slot_page_N = 10;
extern MenuPage<grid_slot_page_N> grid_slot_page;

extern MCLEncoder wavdesign_menu_value_encoder;
extern MCLEncoder wavdesign_menu_entry_encoder;
extern MenuPage<3> wavdesign_menu_page;

extern uint8_t opt_import_src;
extern uint8_t opt_import_dest;
extern uint8_t opt_import_count;
#endif /* MCLMENUS_H__ */
