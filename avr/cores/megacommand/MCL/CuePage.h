/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef CUEPAGE_H__
#define CUEPAGE_H__

#include "GUI.h"

class CuePage : public LightPage {
 public:
 CuePage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL) : LightPage( e1, e2, e3 ,e4) {

 }
 void toggle_cue(int i);
 void toggle_cues_batch();
 void set_level(int curtrack, int value);
 bool handleEvent(gui_event_t *event);
 void display();
 void setup();
 void init();
 void cleanup();
};
#endif /* CUEPAGE_H__ */
