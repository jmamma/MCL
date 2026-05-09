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
 static uint8_t slot_for_note(uint8_t note);

 GridIOPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL) : LightPage( e1, e2, e3 ,e4) {

 }
  void track_select_array_from_type_select(uint8_t *track_select_array);
  void populate_track_select_from_notes(uint8_t *track_select_array);
  void show_group_select_ui(const char *title_P);
  static void draw_popup_P(const char *title_P);
  static void draw_grid_marker(uint8_t body_y_offset);
  static void draw_title(const char *title, uint8_t y_offset = 0);
  static void draw_tbd_panel_header(const char *title, uint8_t y_offset = 0);
  static uint8_t content_y_offset(uint8_t y_offset);
  static void clear_body(uint8_t y_offset);
  static void paint_track_select_leds();
  static bool slot_matches_track_type_select(uint8_t slot);
  virtual void init();
 virtual void cleanup();
 virtual void draw_popup() = 0;
 virtual void group_select() = 0;
 virtual void action() = 0;
 virtual bool handleEvent(gui_event_t *event);
};

#endif /* GRIDIOPAGE_H__ */
