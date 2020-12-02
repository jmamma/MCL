#include "MCL_impl.h"

MCLEncoder loadproj_param1(0, 64, ENCODER_RES_SYS);

LoadProjectPage load_proj_page(&loadproj_param1,&loadproj_param1);

ConvertProjectPage convert_proj_page(&loadproj_param1,&loadproj_param1);

