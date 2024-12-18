/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef FILEBROWSERPAGE_H__
#define FILEBROWSERPAGE_H__

#include "GUI.h"
#include "MCLEncoder.h"
#include "Menu.h"
#include "MenuPage.h"
#include "SdFat.h"
#include "SeqPage.h"

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
#define FM_RECVALL 4
#define FM_SENDALL 5

#define NAME_LENGTH 14

class FileBrowserFileTypes {
  constexpr static uint8_t size = 2;
  char types[size][5];
  uint8_t count = 0;
  public:
  void add(const char *str) {
    if (count < size) {
      strcpy(types[count], str);
      count++;
    }
  }
  void reset() { count = 0; }

  bool compare(char *str) {
    for (uint8_t n = 0; n < count; n++) {
      DEBUG_PRINT("Comparing "); DEBUG_PRINT(str); DEBUG_PRINT(" "); DEBUG_PRINTLN(types[n]);
      if (strcmp(str, types[n]) == 0) {
        DEBUG_PRINTLN("match");
        return true;
      }
    }
    return false;
  }
};

class FileSystemPosition {
  constexpr static uint8_t size = 12;
  uint16_t last_pos[8];
  uint8_t last_cur_row[8];
  uint8_t depth;
  public:
  void push(uint16_t pos, uint8_t row) {
     if (depth < size) {
       last_pos[depth] = pos;
       last_cur_row[depth] = row;
     }
     depth++;
  }
  void pop(uint16_t &pos, uint8_t &row) {
     if (depth != 0) {
       depth--;
       if (depth >= size) {
         goto end;
       }
       pos = last_pos[depth];
       row = last_cur_row[depth];
       return;
     }
     end:
     pos = 1;
     row = 1;
  }
  void reset() { depth = 0; }
};

class FileBrowserPage : public LightPage {
public:
  static File file;
  static int numEntries;

  char lwd[128];
  static char title[12];
  static char str_save[12];
  uint8_t cur_row;
  static uint8_t cur_file;

  // configuration, should be set before calling base init()
  static bool draw_dirs;

  static bool show_dirs;
  static bool select_dirs;
  static bool show_save;
  static bool show_parent;
  static bool show_new_folder;
  static bool show_filemenu;
  static bool show_overwrite;
  static bool show_samplemgr;

  static bool filemenu_active;

  static bool call_handle_filemenu;

  static char focus_match[PRJ_NAME_LEN];
  static FileBrowserFileTypes file_types;

  FileSystemPosition position;

  static bool selection_change;
  static uint16_t selection_change_clock;
  Encoder *param1;
  Encoder *param2;

  FileBrowserPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
                  Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {
    param1 = e1;
    param2 = e2;
    lwd[0] = '\0';
  }
  virtual bool handleEvent(gui_event_t *event);
  void draw_sidebar();
  void draw_filebrowser();
  void draw_menu();

  virtual void display();

  static constexpr uint8_t FILE_TYPE = 0;
  static constexpr uint8_t DIR_TYPE = 1;

  bool add_entry(const char *entry, uint8_t type = FILE_TYPE);
  void get_entry(uint16_t n, char *entry);
  void get_entry(uint16_t n, char *entry, uint8_t &type);

  void draw_scrollbar(uint8_t x_offset);
  bool create_folder();

  bool rm_dir(const char *dir);

  virtual void loop();
  virtual void setup();
  virtual void cleanup();
  virtual void init();

  virtual void on_new() {}
  virtual void on_select(const char *) {}
  virtual void on_delete(const char *);
  virtual void on_rename(const char *from, const char *to);
  // on cancel, the page will be popped,
  // and there's a last chance to clean up.
  virtual void on_cancel() { mcl.popPage(); }
  virtual void chdir_type() {}
  virtual bool _handle_filemenu();

protected:
  void _cd_up();
  bool _cd(const char *);
  void query_filesystem();

private:
  void _calcindices(int &);
};

#endif /* FILEBROWSERPAGE_H__ */
