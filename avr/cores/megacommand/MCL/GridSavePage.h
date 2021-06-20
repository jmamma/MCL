/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef GRIDSAVEPAGE_H__
#define GRIDSAVEPAGE_H__

#include "GridIOPage.h"

class GridSavePage : public GridIOPage {

public:
  GridSavePage(Encoder *e1 = NULL,
               Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL)
      : GridIOPage(e1, e2, e3, e4) {}
  void get_modestr(char *modestr);
  void save();
  bool handleEvent(gui_event_t *event);
  void group_select();
  void loop();
  void display();
  void init();
  void setup();
  void draw_popup();
};

#endif /* GRIDSAVEPAGE_H__ */
