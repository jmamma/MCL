/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef GRIDIOPAGE_H__
#define GRIDIOPAGE_H__

#include "GUI.h"

class GridIOPage : public LightPage {
 public:
 static uint8_t track_type_select;
 static bool show_track_type_select;

 GridIOPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL) : LightPage( e1, e2, e3 ,e4) {

 }
 virtual bool handleEvent(gui_event_t *event);
};

#endif /* GRIDIOPAGE_H__ */
