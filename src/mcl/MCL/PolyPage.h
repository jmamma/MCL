/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef POLYPAGE_H__
#define POLYPAGE_H__

//#include "Pages.h"
#include "GUI.h"
#include "PtcGroups.h"

class PolyPage : public LightPage {
public:
  uint8_t selected_group = PTC_GROUP_LOCAL;

  PolyPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
            Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {
      }

  bool handleEvent(gui_event_t *event);
  void cycle_group(int8_t direction);
  void save_ptc_groups();
  void toggle_group(uint8_t i);
  void draw_mask();
  void display();
  void init();
  void cleanup();
};

extern PolyPage poly_page;

#endif /* POLYPAGE_H__ */
