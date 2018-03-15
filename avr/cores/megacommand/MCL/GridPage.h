/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef GRIDPAGEPAGE_H__
#define GRIDPAGEPAGE_H__
#include "GUI.h"

extern GridEncoder param1(0, GRID_WIDTH - 4, ENCODER_RES_GRID);
extern GridEncoder param2(0, 127, ENCODER_RES_GRID);
extern GridEncoder param3(0, 127, 1);
extern GridEncoder param4(0, 127, 1);

class GridPagePage : LightPage {
public:
  GridPagePage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
               Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {}
  virtual bool handleEvent(gui_event_t *event);
  void displayScroll(uint8_t i);
};

#endif /* GRIDPAGEPAGE_H__ */
