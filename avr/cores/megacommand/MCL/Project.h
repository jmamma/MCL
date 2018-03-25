/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef PROJECT_H__
#define PROJECT_H__
#include "ProjectPages.h"
#include "MCLSysConfig.h"
#define VERSION 2013

class ProjectHeader {
public:
  uint32_t version;
  uint8_t reserved[16];
  uint32_t hash;
  MCLSysConfig cfg;
};

class Project : public ProjectHeader {
public:
  File file;
  void setup();
  bool load_project(char *projectname);
  bool check_project_version();
  bool new_project(char *projectname);
  bool write_header();
};

extern Project proj;

#endif /* PROJECT_H__ */
