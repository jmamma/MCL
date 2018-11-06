/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef LOADPROJECTPAGE_H__
#define LOADPROJECTPAGE_H__

#include "GUI.h"

#define MAX_ENTRIES 64
#ifdef OLED_DISPLAY
#define MAX_VISIBLE_ROWS 4
#else
#define MAX_VISIBLE_ROWS 1
#endif

#define MENU_WIDTH 78

class LoadProjectPage : public LightPage {
public:
  char file_entries[MAX_ENTRIES][16];
  uint8_t numEntries;

  uint8_t cur_col = 0;
  uint8_t cur_row = 0;
  uint8_t cur_proj = 0;
  LoadProjectPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
                  Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {}
  virtual bool handleEvent(gui_event_t *event);
  virtual void display();
  void draw_scrollbar(uint8_t x_offset);
  void loop();
  void setup();
  void init();
};

#endif /* LOADPROJECTPAGE_H__ */
