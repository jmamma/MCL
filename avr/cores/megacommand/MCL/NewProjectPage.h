/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef NEWPROJECTPAGE_H__
#define NEWPROJECTPAGE_H__

#include "GUI.h"

#define FLASH_SPEED 400

class NewProjectPage : public LightPage {
public:
  char newprj[14];
  uint16_t last_clock;

  NewProjectPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
                 Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {}
  virtual void display();
  virtual bool handleEvent(gui_event_t *event);
  void init();
  void setup();
};

#endif /* NEWPROJECTPAGE_H__ */
