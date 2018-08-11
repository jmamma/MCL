#include "AuxPages.h"

extern MCLEncoder mixer_param1(0, 127);
extern MCLEncoder mixer_param2(0, 127);
extern MCLEncoder mixer_param3(0, 8);
MixerPage mixer_page(&mixer_param1, &mixer_param2, &mixer_param3, &mixer_param1);
CuePage cue_page(&mixer_param1, &mixer_param2, &mixer_param3);
MutePage mute_page(&mixer_param1, &mixer_param2, &mixer_param3);
