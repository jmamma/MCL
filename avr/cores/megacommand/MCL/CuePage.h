/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef CUEPAGE_H__
#define CUEPAGE_H__
#include "GUI.h"

class CuePage : LightPage {
 public:
 CuePage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL) : LightPage( e1, e2, e3 ,e4) {

 }
 bool handleEvent(gui_event_t *event);
 void setup();
};
extern CuePage cue_page;
#endif /* CUEPAGE_H__ */
