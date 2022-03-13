/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef FILEBROWSERPAGE_H__
#define FILEBROWSERPAGE_H__

#include "GUI.h"
#include "MCLEncoder.h"
#include "SdFat.h"
#include "SeqPage.h"
#include "Menu.h"
#include "MenuPage.h"

#define MAX_ENTRIES 1024

#ifdef OLED_DISPLAY
#define MAX_VISIBLE_ROWS 4
#else
#define MAX_VISIBLE_ROWS 1
#endif

#define MENU_WIDTH 78

#define MAX_FB_ITEMS 4
#define MAX_FT_SELECT 3

#define FM_CANCEL 0
#define FM_NEW_FOLDER 1
#define FM_DELETE 2
#define FM_RENAME 3
#define FM_OVERWRITE 4
#define FM_RECVALL 5
#define FM_SENDALL 6

#define FILETYPE_WAV 1

class FileBrowserPage : public LightPage {
public:
  static File file;
  static int numEntries;

  static char match[5];
  static char lwd[128];
  static char title[12];
  static uint8_t cur_col;
  static uint8_t cur_row;
  static uint8_t cur_file;

  // configuration, should be set before calling base init()
  static bool show_dirs;
  static bool select_dirs;
  static bool show_save;
  static bool show_parent;
  static bool show_new_folder;
  static bool show_filemenu;
  static bool show_overwrite;

  static bool show_samplemgr;
  static bool show_filetypes;
  static uint8_t filetype_idx;
  static uint8_t filetype_max;
  static const char* filetypes[MAX_FT_SELECT];
  static const char* filetype_names[MAX_FT_SELECT];

  static bool filemenu_active;

  static bool call_handle_filemenu;

  Encoder* param1;
  Encoder* param2;

  FileBrowserPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
                  Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {
          param1 = e1;
          param2 = e2;
      }
  virtual bool handleEvent(gui_event_t *event);
  virtual void display();

  bool add_entry(const char *entry);
  void get_entry(uint16_t n, const char *entry);

  void draw_scrollbar(uint8_t x_offset);
  bool create_folder();

  virtual void loop();
  virtual void setup();
  virtual void init();

  virtual void on_new() {}
  virtual void on_select(const char *) {}
  virtual void on_delete(const char *);
  virtual void on_rename(const char *from, const char *to);
  // on cancel, the page will be popped,
  // and there's a last chance to clean up.
  virtual void on_cancel() { GUI.popPage(); }

  virtual bool _handle_filemenu();
protected:
  void _cd_up();
  void _cd(const char *);

  void query_filesystem();

private:

  void _calcindices(int &);
};

#endif /* FILEBROWSERPAGE_H__ */
