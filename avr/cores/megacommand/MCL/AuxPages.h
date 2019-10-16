/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef AUXPAGES_H__
#define AUXPAGES_H__

#include "MCLEncoder.h"
#include "MixerPage.h"
#include "RoutePage.h"
#include "RAMPage.h"

extern MCLEncoder mixer_param1;
extern MCLEncoder mixer_param2;
extern MCLEncoder mixer_param3;
extern MCLEncoder mixer_param4;
extern MCLEncoder route_param1;
extern MCLEncoder route_param2;

extern MixerPage mixer_page;
extern RoutePage route_page;

extern RAMPage ram_page_a;
extern RAMPage ram_page_b;
#endif /* AUXPAGES_H__ */
