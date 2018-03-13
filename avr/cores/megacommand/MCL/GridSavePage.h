/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef GRIDSAVEPAGE_H__
#define GRIDSAVEPAGE_H__
#include "GUI.h"

class GridSavePage : public GridIOPage {

public:
  GridSavePage(Encoder *e1 = NULL,
               Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL)
      : GridIOPage(e1, e2, e3, e4) {}
  bool handleEvent(gui_event_t *event);
  bool displayPage();
  void setup();
};

extern GridSavePage grid_save_page;
#endif /* GRIDSAVEPAGE_H__ */
