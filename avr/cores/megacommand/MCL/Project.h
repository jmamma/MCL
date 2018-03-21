/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef PROJECT_H__
#define PROJECT_H__
#include "ProjectPages.h"
#include "MCL.h"
#define VERSION 2013

class ProjectHeader {
  uint32_t version;
  uint8_t reserved[16];
  uint32_t hash;
  MCLSysConfig cfg;
  write_header();
};

class Project {
public:
  File file;
  ProjectHeader header;
  void setup();
  bool sd_load_project(char *projectname);
  bool check_project_version();
  bool sd_new_project(char *projectname);
};

extern Project proj;

#endif /* PROJECT_H__ */
