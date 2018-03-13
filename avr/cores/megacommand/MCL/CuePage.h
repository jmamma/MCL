/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef CUEPAGE_H__
#define CUEPAGE_H__
#include "GUI.h"

class CuePage : LightPage {
 public:
 CuePage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL) : FlexPage( e1, e2, e3 ,e4) {

 }
 virtual bool handleEvent(gui_event_t *event);
};

#endif /* CUEPAGE_H__ */
