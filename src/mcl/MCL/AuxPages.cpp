#include "MCLEncoder.h"
#include "PerfEncoder.h"
#include "RoutePage.h"
#include "MixerPage.h"
#include "FXPage.h"
#include "LFOPage.h"
#include "PerfPage.h"
#include "MCLSeq.h"

MCLEncoder mixer_param1;
MCLEncoder mixer_param2;
MCLEncoder mixer_param3;
MCLEncoder mixer_param4;

MCLEncoder route_param1(2, 5);
MCLExpEncoder route_param2(1, 64);

MCLEncoder fx_param1;
MCLEncoder fx_param2;
MCLEncoder fx_param3;
MCLEncoder fx_param4;

PerfEncoder perf_param1(0,127);
PerfEncoder perf_param2(0,127);
PerfEncoder perf_param3(0,127);
PerfEncoder perf_param4(0,127);

MixerPage mixer_page(&perf_param1, &perf_param2, &perf_param3,
                     &perf_param4);
RoutePage route_page(&route_param1, &route_param2);

fx_param_t fx_echo_params[8] = {
    {MD_FX_ECHO, MD_ECHO_TIME},
    {MD_FX_ECHO, MD_ECHO_FB},
    {MD_FX_ECHO, MD_ECHO_FLTF},
    {MD_FX_ECHO, MD_ECHO_FLTW},
    {MD_FX_ECHO, MD_ECHO_MOD},
    {MD_FX_ECHO, MD_ECHO_MFRQ},
    {MD_FX_ECHO, MD_ECHO_MONO},
    {MD_FX_ECHO, MD_ECHO_LEV}};

fx_param_t fx_reverb_params[8] = {
    {MD_FX_REV, MD_REV_DVOL},
    {MD_FX_REV, MD_REV_DEC},
    {MD_FX_REV, MD_REV_HP},
    {MD_FX_REV, MD_REV_LP},
    {MD_FX_REV, MD_REV_DAMP},
    {MD_FX_REV, MD_REV_GATE},
    {MD_FX_REV, MD_REV_PRED},
    {MD_FX_REV, MD_REV_LEV}};


FXPage fx_page_a(&fx_param1, &fx_param2, &fx_param3, &fx_param4,
                 (fx_param_t*) &fx_echo_params, 8, "ECHO", 0);

FXPage fx_page_b(&fx_param1, &fx_param2, &fx_param3, &fx_param4,
                 (fx_param_t*) &fx_reverb_params, 8, "REVERB", 1);

LFOPage lfo_page(&(mcl_seq.lfo_tracks[0]), &fx_param1, &fx_param2, &fx_param3, &fx_param4);

PerfPage perf_page(&perf_param1, &fx_param2, &fx_param3, &fx_param4);

