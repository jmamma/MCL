/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef LOADPROJECTPAGE_H__
#define LOADPROJECTPAGE_H__

#include "GUI.h"

class LoadProjectPage : public LightPage {
public:
  char file_entries[30][16];
  LoadProjectPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
                  Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {}
  virtual bool handleEvent(gui_event_t *event);
  virtual void display();
  void setup();
};

#endif /* LOADPROJECTPAGE_H__ */
