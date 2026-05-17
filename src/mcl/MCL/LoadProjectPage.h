/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef LOADPROJECTPAGE_H__
#define LOADPROJECTPAGE_H__

#include "FileBrowserPage.h"
#include "GUI.h"

#define MAX_VISIBLE_ROWS 4

#define MENU_WIDTH 78

class LoadProjectPage : public FileBrowserPage {
public:
  const uint8_t f_len = PRJ_NAME_LEN + 5;
  LoadProjectPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
                  Encoder *e4 = NULL)
      : FileBrowserPage(e1, e2, e3, e4) {}
  virtual void on_select(const char *entry);
  virtual void on_delete(const char *entry);
  virtual void on_rename( const char *from, const char *to);
  virtual void setup();
  virtual void init();
  virtual void cleanup();
#ifdef PLATFORM_TBD
  virtual bool tbd_can_cd_up() const;
#endif

protected:
  virtual bool can_show_parent_entry() const;
  virtual uint8_t entry_type_for_dir(const char *entry);

private:
  bool build_project_path(const char *entry, char *out, size_t out_len) const;
  bool current_project_parent(const char **parent) const;
  bool is_project_dir(const char *entry) const;
  void focus_current_project();
};

#endif /* LOADPROJECTPAGE_H__ */
