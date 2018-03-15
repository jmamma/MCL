/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef GRIDWRITEPAGE_H__
#define GRIDWRITEPAGE_H__
#include "GUI.h"

class GridWritePage : GridIOPage {
 public:
 GridWritePage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL) : GridIOPage(e1, e2, e3 ,e4) {

 }
 bool handleEvent(gui_event_t *event);
 bool display();
 void setup();
};

#endif /* GRIDWRITEPAGE_H__ */
