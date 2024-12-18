#include "MCL.h"
#include "Menu.h"

PageSelectEntry Entries[] = {
    {"GRID", GRID_PAGE, 0, 0, 24, 15, nullptr},
    {"MIXER", MIXER_PAGE, 1, 0, 24, 16, nullptr},
    {"PERF", PERF_PAGE_0, 2, 0, 24, 18, nullptr},
    {"ROUTE", ROUTE_PAGE, 3, 0, 24, 14, nullptr},

    {"STEP EDIT", SEQ_STEP_PAGE, 4, 1, 24, 21, nullptr},
    {"LFO", LFO_PAGE, 5, 1, 24, 24, nullptr},
    {"PIANO ROLL", SEQ_EXTSTEP_PAGE, 6, 1, 24, 25, nullptr},
    {"CHROMATIC", SEQ_PTC_PAGE, 7, 1, 24, 25, nullptr},
#ifdef SOUND_PAGE
    {"SAMPLE MANAGER", SAMPLE_BROWSER, 8, 2, 24, 25, nullptr},
#endif
#ifdef WAV_DESIGNER
    {"WAV DESIGNER", WD_PAGE_0, 9, 2, 24, 19, nullptr},
#endif
    {"DELAY", FX_PAGE_A, 12, 3, 24, 25, nullptr},
    {"REVERB", FX_PAGE_B, 13, 3, 24, 25, nullptr},
    {"RAM-1", RAM_PAGE_A, 14, 3, 24, 25, nullptr},
    {"RAM-2", RAM_PAGE_B, 15, 3, 24, 25, nullptr},
};
