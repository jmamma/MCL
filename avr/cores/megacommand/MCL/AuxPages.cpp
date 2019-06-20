#include "AuxPages.h"

extern MCLEncoder mixer_param1(0, 127);
extern MCLEncoder mixer_param2(0, 127);
extern MCLEncoder mixer_param3(0, 127);
extern MCLEncoder mixer_param4(0, 127);

extern MCLEncoder route_param1(2,5);
extern MCLEncoder route_param2(0,6);
MixerPage mixer_page(&mixer_param1, &mixer_param2, &mixer_param3, &mixer_param4);
RoutePage route_page(&route_param1, &route_param2, &route_param2);
