/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef COMMONPAGES_H__
#define COMMONPAGES_H__

#include "GUI/Pages/Sequencer/LFOPage.h"
#include "GUI/MCLEncoder.h"
#include "GUI/Pages/Performance/MixerPage.h"
#include "GUI/Pages/Performance/PerfPage.h"

extern MCLEncoder mixer_param1;
extern MCLEncoder mixer_param2;
extern MCLEncoder mixer_param3;
extern MCLEncoder mixer_param4;

extern MCLEncoder fx_param1;
extern MCLEncoder fx_param2;
extern MCLEncoder fx_param3;
extern MCLEncoder fx_param4;

extern PerfEncoder perf_param1;
extern PerfEncoder perf_param2;
extern PerfEncoder perf_param3;
extern PerfEncoder perf_param4;

extern MixerPage mixer_page;
extern LFOPage lfo_page;
extern PerfPage perf_page;

#endif /* COMMONPAGES_H__ */
