/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef POLYPAGE_H__
#define POLYPAGE_H__

//#include "Pages.h"
#include "GUI.h"

class PolyPage : public LightPage {
public:
  uint16_t *poly_mask;

  PolyPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
            Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {
      }

  bool handleEvent(gui_event_t *event);
  void toggle_mask(uint8_t i);
  void draw_mask();
  void display();
  void setup();
  void init();
  void cleanup();
};

extern PolyPage poly_page;

#endif /* POLYPAGE_H__ */
