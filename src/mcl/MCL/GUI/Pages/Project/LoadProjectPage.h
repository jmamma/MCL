/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef LOADPROJECTPAGE_H__
#define LOADPROJECTPAGE_H__

#include "GUI/Pages/Project/FileBrowserPage.h"
#include "GUI.h"

#define MAX_VISIBLE_ROWS 4

#define MENU_WIDTH 78

class LoadProjectPage : public FileBrowserPage {
public:
  LoadProjectPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
                  Encoder *e4 = NULL)
      : FileBrowserPage(e1, e2, e3, e4) {}
  virtual void on_select(const char *entry);
  virtual void on_delete(const char *entry);
  virtual void on_rename( const char *from, const char *to);
  virtual void on_copy(const char *from, const char *to);
  virtual void on_new();
  virtual void on_cancel();
  virtual bool handleEvent(gui_event_t *event);
  virtual void setup();
  virtual void init();
  virtual void cleanup();
#ifdef PLATFORM_TBD
  virtual bool tbd_can_cd_up() const;
#endif

protected:
  virtual bool can_show_parent_entry() const;
  virtual uint8_t entry_type_for_dir(const char *entry);
  virtual uint8_t resolve_entry_type(uint16_t n, const char *entry,
                                     uint8_t type);
  virtual bool _handle_filemenu();

private:
  bool build_project_path(const char *entry, char *out, size_t out_len) const;
  bool current_project_parent(const char **parent) const;
#ifdef MCL_HAS_FILE_MOVE
  bool enter_project_move_destination(const char *entry);
  bool move_to_current_folder();
#endif
  bool is_project_dir(const char *entry) const;
  void focus_current_project();
};

#endif /* LOADPROJECTPAGE_H__ */
