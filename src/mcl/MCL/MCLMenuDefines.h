#pragma once

#if defined(__AVR__)
constexpr size_t boot_menu_page_N = 4;
#else
constexpr size_t boot_menu_page_N = 2;
#endif
constexpr size_t start_menu_page_N = 2;
constexpr size_t system_menu_page_N = 6;
constexpr size_t midi_config_page_N = 6;
constexpr size_t md_config_page_N = 2;
constexpr size_t mcl_config_page_N = 1;
constexpr size_t chain_config_page_N = 3;
constexpr size_t aux_config_page_N = 2;
constexpr size_t md_import_page_N = 4;
constexpr size_t midiport_menu_page_N = 6;
constexpr size_t midiprogram_menu_page_N = 3;
constexpr size_t midiclock_menu_page_N = 4;
constexpr size_t midiroute_menu_page_N = 4;
constexpr size_t midimachinedrum_menu_page_N = 3;
constexpr size_t midigeneric_menu_page_N = 1;
constexpr size_t file_menu_page_N = 6;
constexpr size_t seq_menu_page_N = 25;
constexpr size_t grid_slot_page_N = 9;
constexpr size_t wavdesign_menu_page_N = 3;
constexpr size_t perf_menu_page_N = 2;
