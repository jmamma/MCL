#include "CommonPages.h"

#include "MCLSeq.h"

MCLEncoder mixer_param1(0, 127, 1, ENCODER_FAST_SPEED);
MCLEncoder mixer_param2(0, 127, 1, ENCODER_FAST_SPEED);
MCLEncoder mixer_param3(0, 127, 1, ENCODER_FAST_SPEED);
MCLEncoder mixer_param4(0, 127, 1, ENCODER_FAST_SPEED);

MCLEncoder fx_param1(0, 127, 1, ENCODER_FAST_SPEED);
MCLEncoder fx_param2(0, 127, 1, ENCODER_FAST_SPEED);
MCLEncoder fx_param3(0, 127, 1, ENCODER_FAST_SPEED);
MCLEncoder fx_param4(0, 127, 1, ENCODER_FAST_SPEED);

PerfEncoder perf_param1(0, 127, 1, ENCODER_FAST_SPEED);
PerfEncoder perf_param2(0, 127, 1, ENCODER_FAST_SPEED);
PerfEncoder perf_param3(0, 127, 1, ENCODER_FAST_SPEED);
PerfEncoder perf_param4(0, 127, 1, ENCODER_FAST_SPEED);

MixerPage mixer_page(&perf_param1, &perf_param2, &perf_param3, &perf_param4);

LFOPage lfo_page(&(mcl_seq.lfo_tracks[0]), &fx_param1, &fx_param2, &fx_param3,
                 &fx_param4);

PerfPage perf_page(&perf_param1, &fx_param2, &fx_param3, &fx_param4);
