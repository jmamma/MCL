/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef POLYPAGE_H__
#define POLYPAGE_H__

//#include "Pages.h"
#include "GUI.h"
#include "PtcGroups.h"

class PolyPage : public LightPage {
public:
  uint8_t selected_group = PTC_GROUP_OFF;
  uint8_t first_held_track = 255;
  uint16_t selected_tracks;

  PolyPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
            Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {
      }

  bool handleEvent(gui_event_t *event);
  void cycle_group(int8_t direction);
  void save_ptc_groups();
  void apply_selected_group();
  void release_track(uint8_t i);
  void press_track(uint8_t i);
  void draw_mask();
  void display();
  void init();
  void cleanup();
  void loop();
};

extern PolyPage poly_page;

#endif /* POLYPAGE_H__ */
