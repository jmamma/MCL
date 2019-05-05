#include "AuxPages.h"

extern MCLEncoder mixer_param1(0, 127);
extern MCLEncoder mixer_param2(0, 127);
extern MCLEncoder mixer_param3(0, 127);
extern MCLEncoder mixer_param4(0, 127);

extern MCLEncoder mute_param1(0, 7);

MixerPage mixer_page(&mixer_param1, &mixer_param2, &mixer_param3, &mixer_param4);
CuePage cue_page(&mute_param1, &mute_param1, &mute_param1);
MutePage mute_page(&mute_param1, &mute_param1, &mute_param1);
