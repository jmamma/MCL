/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef TEXTINPUTPAGE_H__
#define TEXTINPUTPAGE_H__

#include "GUI.h"

#define FLASH_SPEED 400

class TextInputPage : public LightPage {
public:
  char *textp;
  const char *title;
  char text[17];
  uint8_t length;
  uint8_t max_length;
  bool return_state;
  uint16_t last_clock;
  bool normal_mode;
  uint8_t cursor_position;
  TextInputPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
                 Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {}
  virtual bool handleEvent(gui_event_t *event);

  void config_normal();
  void config_charpane();
  void display();
  void display_normal();
  void display_charpane();
  void init();
  void init_text(char *text_, const char *title_, uint8_t len);
  void loop();
  void setup();
  void update_char();
};

#endif /* TEXTINPUTPAGE_H__ */
