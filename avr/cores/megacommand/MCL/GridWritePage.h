/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef GRIDWRITEPAGE_H__
#define GRIDWRITEPAGE_H__

#include "GridIOPage.h"

class GridWritePage : public GridIOPage {
 public:
 GridWritePage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL) : GridIOPage(e1, e2, e3 ,e4) {

 }
 bool handleEvent(gui_event_t *event);
 void display();
 void setup();
};

#endif /* GRIDWRITEPAGE_H__ */
