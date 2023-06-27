/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef AUXPAGES_H__
#define AUXPAGES_H__

#include "MCLEncoder.h"
#include "LFOPage.h"
#include "MixerPage.h"
#include "RoutePage.h"
#include "RAMPage.h"
#include "FXPage.h"
#include "PerfPage.h"
#include "MD.h"

extern MCLEncoder mixer_param1;
extern MCLEncoder mixer_param2;
extern MCLEncoder mixer_param3;
extern MCLEncoder mixer_param4;
extern MCLEncoder route_param1;
extern MCLExpEncoder route_param2;

extern MixerPage mixer_page;
extern RoutePage route_page;

extern RAMPage ram_page_a;
extern RAMPage ram_page_b;

extern MCLEncoder fx_param1;
extern MCLEncoder fx_param2;
extern MCLEncoder fx_param3;
extern MCLEncoder fx_param4;

extern PerfEncoder perf_param1;
extern PerfEncoder perf_param2;
extern PerfEncoder perf_param3;
extern PerfEncoder perf_param4;

extern FXPage fx_page_a;
extern FXPage fx_page_b;

extern LFOPage lfo_page;
extern PerfPage perf_page;

#endif /* AUXPAGES_H__ */
