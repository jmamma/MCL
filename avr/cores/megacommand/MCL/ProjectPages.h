/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef PROJECTPAGES_H__
#define PROJECTPAGES_H__


class ProjectPages {
public:
  void setup();
};

extern MCLEncoder loadproj_param1(1, 64, ENCODER_RES_SYS);

extern MCLEncoder newproj_param1(1, 10, ENCODER_RES_SYS);
extern MCLEncoder newproj_param2(0, 36, ENCODER_RES_SYS);
extern MCLEncoder newproj_param4(0, 127, ENCODER_RES_SYS);


extern NewProjectPage new_project_page(&newproj_param1, &newproj_param2);

extern LoadProjectPage load_project_page(&loadproj_param1);

#endif /* PROJECTPAGES_H__ */
