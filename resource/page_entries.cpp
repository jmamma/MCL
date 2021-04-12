#include "MCL.h"
#include "MCL_impl.h"

PageSelectEntry Entries[] = {
    {"GRID", &grid_page, 0, 0, 24, 15, (uint8_t *)icon_grid},
    {"MIXER", &mixer_page, 1, 0, 24, 16, (uint8_t *)icon_mixer},
    {"ROUTE", &route_page, 2, 0, 24, 16, (uint8_t *)icon_route},
    {"LFO", &lfo_page, 3, 0, 24, 24, (uint8_t *)icon_lfo},

    {"STEP EDIT", &seq_step_page, 4, 1, 24, 25, (uint8_t *)icon_step},
    {"PIANO ROLL", &seq_extstep_page, 6, 1, 24, 25, (uint8_t *)icon_pianoroll},
    {"LOCKS", &seq_param_page[0], 5, 1, 24, 19, (uint8_t *)icon_para},
    {"CHROMATIC", &seq_ptc_page, 7, 1, 24, 25, (uint8_t *)icon_chroma},
#ifdef SOUND_PAGE
    {"SOUND MANAGER", &sound_browser, 8, 2, 24, 19, (uint8_t *)icon_sound},
#endif
#ifdef WAV_DESIGNER
    {"WAV DESIGNER", &wd.pages[0], 9, 2, 24, 19, (uint8_t *)icon_wavd},
#endif
#ifdef LOUDNESS_PAGE
    {"LOUDNESS", &loudness_page, 10, 2, 24, 16, (uint8_t *)icon_loudness},
#endif
    {"DELAY", &fx_page_a, 12, 3, 24, 25, (uint8_t *)icon_rhytmecho},
    {"REVERB", &fx_page_b, 13, 3, 24, 25, (uint8_t *)icon_gatebox},
    {"RAM-1", &ram_page_a, 14, 3, 24, 25, (uint8_t *)icon_ram1},
    {"RAM-2", &ram_page_b, 15, 3, 24, 25, (uint8_t *)icon_ram2},
};

