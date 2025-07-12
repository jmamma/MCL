#pragma once
#include "MCLDefines.h"

enum PageIndex {
    // Core pages
    GRID_PAGE = 0,
    PAGE_SELECT_PAGE,
    SYSTEM_PAGE,
    MIXER_PAGE,
    GRID_SAVE_PAGE,
    GRID_LOAD_PAGE,
    // Main sequence pages
    SEQ_STEP_PAGE,
    SEQ_EXTSTEP_PAGE,
    SEQ_PTC_PAGE,
    // UI pages
    TEXT_INPUT_PAGE,
    POLY_PAGE,
    SAMPLE_BROWSER,
    QUESTIONDIALOG_PAGE,
    START_MENU_PAGE,
    BOOT_MENU_PAGE,
    // Effect pages
    FX_PAGE_A,
    FX_PAGE_B,
    ROUTE_PAGE,
    LFO_PAGE,
    // Memory pages
    RAM_PAGE_A,
    RAM_PAGE_B,
    // Configuration pages
    LOAD_PROJ_PAGE,
    MIDI_CONFIG_PAGE,
    MD_CONFIG_PAGE,
    CHAIN_CONFIG_PAGE,
    AUX_CONFIG_PAGE,
    MCL_CONFIG_PAGE,
    // Additional feature pages
    ARP_PAGE,
    MD_IMPORT_PAGE,
    // MIDI menu pages
    MIDIPORT_MENU_PAGE,
    MIDIPROGRAM_MENU_PAGE,
    MIDICLOCK_MENU_PAGE,
    MIDIROUTE_MENU_PAGE,
    MIDIMACHINEDRUM_MENU_PAGE,
    MIDIGENERIC_MENU_PAGE,
    // Browser pages
    SOUND_BROWSER,
    // Performance page
    PERF_PAGE_0,
#ifdef WAV_DESIGNER
    // WAV Designer pages - grouped together at the end
    WD_MIXER_PAGE,
    WD_PAGE_0,
    WD_PAGE_1,
    WD_PAGE_2,
#endif
    // Special values
    NUM_PAGES,  // Automatically tracks total number of pages
    NULL_PAGE = 255
};

