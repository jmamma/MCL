#include "MCL.h"
#include "MCL_impl.h"

PageSelectEntry Entries[] = {
    {"GRID", &grid_page, 0, 0, 24, 15, nullptr},
    {"MIXER", &mixer_page, 1, 0, 24, 16, nullptr},
    {"ROUTE", &route_page, 2, 0, 24, 16, nullptr},
    {"LFO", &lfo_page, 3, 0, 24, 24, nullptr},

    {"STEP EDIT", &seq_step_page, 4, 1, 24, 25, nullptr},
    {"PIANO ROLL", &seq_extstep_page, 6, 1, 24, 25, nullptr},
    {"LOCKS", &seq_param_page[0], 5, 1, 24, 19, nullptr},
    {"CHROMATIC", &seq_ptc_page, 7, 1, 24, 25, nullptr},
#ifdef SOUND_PAGE
    {"SOUND MANAGER", &sound_browser, 8, 2, 24, 19, nullptr},
#endif
#ifdef WAV_DESIGNER
    {"WAV DESIGNER", &wd.pages[0], 9, 2, 24, 19, nullptr},
#endif
#ifdef LOUDNESS_PAGE
    {"LOUDNESS", &loudness_page, 10, 2, 24, 16, nullptr},
#endif
    {"DELAY", &fx_page_a, 12, 3, 24, 25, nullptr},
    {"REVERB", &fx_page_b, 13, 3, 24, 25, nullptr},
    {"RAM-1", &ram_page_a, 14, 3, 24, 25, nullptr},
    {"RAM-2", &ram_page_b, 15, 3, 24, 25, nullptr},
};

