/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MCLMENUS_H__
#define MCLMENUS_H__

#include "MCLEncoder.h"
#include "MCLSysConfig.h"
#include "MenuPage.h"
#include "TextInputPage.h"

//#include "ProjectPages.h"
//#include "PolyPage.h"
//#include "GridPages.h"
//#include "TextInputPage.h"
//#include "SeqPages.h"
//#include "DiagnosticPage.h"
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

extern MCLEncoder config_param9;
extern MCLEncoder config_param10;
extern MCLEncoder config_param11;
extern MCLEncoder config_param12;
extern MCLEncoder config_param13;
extern MCLEncoder config_param14;

extern void new_proj_handler();

constexpr size_t boot_menu_page_N = 4;
extern MenuPage<boot_menu_page_N> boot_menu_page;

constexpr size_t start_menu_page_N = 2;
extern MenuPage<start_menu_page_N> start_menu_page;

constexpr size_t system_menu_page_N = 6;
extern MenuPage<system_menu_page_N> system_page;

constexpr size_t midi_config_page_N = 6;
extern MenuPage<midi_config_page_N> midi_config_page;

constexpr size_t md_config_page_N = 2;
extern MenuPage<md_config_page_N> md_config_page;

constexpr size_t mcl_config_page_N = 1;
extern MenuPage<mcl_config_page_N> mcl_config_page;

constexpr size_t chain_config_page_N = 3;
extern MenuPage<chain_config_page_N> chain_config_page;

constexpr size_t aux_config_page_N = 2;
extern MenuPage<aux_config_page_N> aux_config_page;

constexpr size_t md_import_page_N = 4;
extern MenuPage<md_import_page_N> md_import_page;

constexpr size_t midiport_menu_page_N = 6;
extern MenuPage<midiport_menu_page_N> midiport_menu_page;

constexpr size_t midiprogram_menu_page_N = 3;
extern MenuPage<midiprogram_menu_page_N> midiprogram_menu_page;

constexpr size_t midiclock_menu_page_N = 4;
extern MenuPage<midiclock_menu_page_N> midiclock_menu_page;

constexpr size_t midiroute_menu_page_N = 4;
extern MenuPage<midiroute_menu_page_N> midiroute_menu_page;

constexpr size_t midimachinedrum_menu_page_N = 3;
extern MenuPage<midimachinedrum_menu_page_N> midimachinedrum_menu_page;

constexpr size_t midigeneric_menu_page_N = 1;
extern MenuPage<midigeneric_menu_page_N> midigeneric_menu_page;

extern MCLEncoder input_encoder1;
extern MCLEncoder input_encoder2;

extern MCLEncoder file_menu_encoder;

constexpr size_t file_menu_page_N = 6;
extern MenuPage<file_menu_page_N> file_menu_page;

extern MCLEncoder seq_menu_value_encoder;
extern MCLEncoder seq_menu_entry_encoder;

constexpr size_t seq_menu_page_N = 25;
extern MenuPage<seq_menu_page_N> seq_menu_page;

extern MCLEncoder grid_slot_param1;
extern MCLEncoder grid_slot_param2;

constexpr size_t grid_slot_page_N = 9;
extern MenuPage<grid_slot_page_N> grid_slot_page;

extern MCLEncoder wavdesign_menu_value_encoder;
extern MCLEncoder wavdesign_menu_entry_encoder;

constexpr size_t wavdesign_menu_page_N = 3;
extern MenuPage<wavdesign_menu_page_N> wavdesign_menu_page;

constexpr size_t perf_menu_page_N = 2;
extern MenuPage<perf_menu_page_N> perf_menu_page;

extern TextInputPage text_input_page;

extern uint8_t opt_import_src;
extern uint8_t opt_import_dest;
extern uint8_t opt_import_count;
#endif /* MCLMENUS_H__ */

