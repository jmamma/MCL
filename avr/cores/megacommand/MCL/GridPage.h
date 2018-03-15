/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef GRIDPAGEPAGE_H__
#define GRIDPAGEPAGE_H__
#include "GUI.h"

class GridPage : LightPage {
public:
  GridPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
               Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {}
  virtual bool handleEvent(gui_event_t *event);
  void displayScroll(uint8_t i);
};

#endif /* GRIDPAGEPAGE_H__ */
