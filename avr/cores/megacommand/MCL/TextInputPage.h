/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef TEXTINPUTPAGE_H__
#define TEXTINPUTPAGE_H__

#include "GUI.h"

#define FLASH_SPEED 400

class TextInputPage : public LightPage {
public:
  char *textp;
  char *title;
  char text[17];
  uint8_t length;
  bool return_state;
  uint16_t last_clock;

  TextInputPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
                 Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {}
  virtual bool handleEvent(gui_event_t *event);
  void display();
  void init();
  void init_text(char *text_, char *title_, uint8_t len);
  void setup();
  void update_char();
};

#endif /* TEXTINPUTPAGE_H__ */
