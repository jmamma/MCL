/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef GRIDIOPAGE_H__
#define GRIDIOPAGE_H__

#include "GUI.h"

class GridIOPage : public LightPage {
 public:
 static uint32_t track_select;
 static uint8_t old_grid;

 static bool show_track_type;
 static bool show_offset;
 static uint8_t offset;

 GridIOPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL) : LightPage( e1, e2, e3 ,e4) {

 }
 void track_select_array_from_type_select(uint8_t *track_select_array);
 virtual void init();
 virtual void cleanup();
 virtual void draw_popup() = 0;
 virtual void group_select() = 0;
 virtual void action() = 0;
 virtual bool handleEvent(gui_event_t *event);
};

#endif /* GRIDIOPAGE_H__ */
