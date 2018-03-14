/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SYSTEMPAGE_H__
#define SYSTEMPAGE_H__
#include "GUI.h"

class SystemPage : LightPage {
 public:
 SystemPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL) : LightPage( e1, e2, e3 ,e4) {

 }
 virtual bool handleEvent(gui_event_t *event);
};

#endif /* SYSTEMPAGE_H__ */
