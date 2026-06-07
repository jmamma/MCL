/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef FILEBROWSERPAGE_H__
#define FILEBROWSERPAGE_H__

#include "GUI.h"
#include "MCLEncoder.h"
#include "Menu.h"
#include "MenuPage.h"
#include "MCLDefines.h"
#include "SeqPage.h"
#include "MCLSd.h"

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
#define FM_RENAME 2
#define FM_MOVE 3
#define FM_DUPLICATE 4
#define FM_VERSIONS 5
#define FM_DELETE 6
#define FM_RECVALL 7
#define FM_SENDALL 8

#define FM_MASK(entry) (1U << (entry))

#define NAME_LENGTH 14

class FileBrowserFileTypes {
  constexpr static uint8_t size = 2;
  const char *types[size];
  uint8_t count;
  public:
  void add(const char *str) {
    if (count < size) {
      types[count++] = str;
    }
  }
  void reset() { count = 0; }

  bool compare(const char *str) {
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
  constexpr static uint8_t size = 8;
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
  static char str_save[16];
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
  static bool show_samplemgr;
  static bool show_copy;
  static bool show_move;
  static bool show_versions;

  static bool filemenu_active;
#ifdef MCL_HAS_FILE_MOVE
  static bool move_destination_mode;
  static char move_source_path[PRJ_PATH_LEN];
#else
  static constexpr bool move_destination_mode = false;
#endif

  static char focus_match[FILE_ENTRY_SIZE];
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
  void open_filemenu();

  virtual void display();

  static constexpr uint8_t FILE_TYPE = 0;
  static constexpr uint8_t DIR_TYPE = 1;
  static constexpr uint8_t UNKNOWN_DIR_TYPE = 2;
  static constexpr uint8_t SKIP_TYPE = 255;

  bool add_entry(const char *entry, uint8_t type = FILE_TYPE);
  void get_entry(uint16_t n, char *entry);
  void get_entry(uint16_t n, char *entry, uint8_t &type);
  void set_entry_type(uint16_t n, uint8_t type);

  void draw_scrollbar(uint8_t x_offset);
  bool create_folder();

  bool rm_dir(const char *dir);

  virtual void loop();
  virtual void setup();
  virtual void init();
  static void reset_browser_options();

  virtual void on_new() {
#ifdef MCL_HAS_FILE_MOVE
    if (move_destination_mode) {
      move_to_current_folder();
    }
#endif
  }
  virtual void on_select(const char *) {}
  virtual void on_delete(const char *);
  virtual void on_rename(const char *from, const char *to);
  virtual void on_copy(const char *from, const char *to);
  // on cancel, the page will be popped,
  // and there's a last chance to clean up.
  virtual void on_cancel() { mcl.popPage(); }
  virtual uint8_t resolve_entry_type(uint16_t n, const char *entry,
                                     uint8_t type) {
    (void)n;
    (void)entry;
    return type;
  }
  virtual bool _handle_filemenu();
#ifdef PLATFORM_TBD
  virtual bool tbd_can_cd_up() const;
  virtual bool tbd_cd_up();
#endif

protected:
  static void build_delete_message(char *dst, uint8_t dst_len,
                                   const char *entry);
  void _cd_up();
  bool _cd(const char *);
  void query_filesystem();
  virtual bool can_show_parent_entry() const;
  virtual uint8_t entry_type_for_dir(const char *entry);
#ifdef MCL_HAS_FILE_MOVE
  bool enter_move_destination(const char *entry);
  bool start_move_destination(const char *source_path);
  bool move_to_current_folder();
  bool finish_move_to_path(const char *dest_path);
  void cancel_move_destination();
#endif
  static bool path_starts_with_dir(const char *path, const char *dir);

private:
  void _calcindices(int &);
};

#endif /* FILEBROWSERPAGE_H__ */
