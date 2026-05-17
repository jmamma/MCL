/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef PROJECTVERSIONPAGE_H__
#define PROJECTVERSIONPAGE_H__

#include "FileBrowserPage.h"
#include "MCLMemory.h"

class ProjectVersionPage : public FileBrowserPage {
public:
  ProjectVersionPage(Encoder *e1 = NULL, Encoder *e2 = NULL,
                     Encoder *e3 = NULL, Encoder *e4 = NULL)
      : FileBrowserPage(e1, e2, e3, e4) {}

  void set_project(const char *project);
  virtual void setup();
  virtual void init();
  virtual bool handleEvent(gui_event_t *event);
  virtual void on_new();
  virtual void on_select(const char *entry);
  virtual void on_delete(const char *entry);

private:
  static constexpr uint8_t VERSION_ENTRY_BASE = 16;
  char project_path[PRJ_PATH_LEN];
  char project_name[PRJ_NAME_LEN + 1];

  void query_versions();
  bool selected_pair(uint8_t *pair);
};

#endif /* PROJECTVERSIONPAGE_H__ */
