#include "ProjectPages.h"
#include "MCL.h"

MCLEncoder loadproj_param1(0, 64, ENCODER_RES_SYS);

MCLEncoder newproj_param1(1, 10, ENCODER_RES_SYS);
MCLEncoder newproj_param2(0, 36, ENCODER_RES_SYS);


NewProjectPage new_proj_page(&newproj_param1, &newproj_param2);
LoadProjectPage load_proj_page(&loadproj_param1,&loadproj_param1);

