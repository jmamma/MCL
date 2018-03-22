/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef PROJECTPAGES_H__
#define PROJECTPAGES_H__

#include "MCLEncoder.h"
#include "NewProjectPage.h"
#include "LoadProjectPage.h"

class ProjectPages {
public:
  void setup();
};

extern MCLEncoder loadproj_param1;

extern MCLEncoder newproj_param1;
extern MCLEncoder newproj_param2;
extern MCLEncoder newproj_param4;


extern NewProjectPage new_project_page;

extern LoadProjectPage load_project_page;

#endif /* PROJECTPAGES_H__ */
