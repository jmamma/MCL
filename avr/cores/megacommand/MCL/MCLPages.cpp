#include "MCLPages.h"
#include "MCLSystemPage.h"

MCLEncoder options_param1(0, 7, ENCODER_RES_SYS);
MCLEncoder options_param2(0, 17, ENCODER_RES_SYS);
MCLSystemPage system_page(&options_param1, &options_param2);
