/* Yatao Li yatao.li@live.com 2019 */

#ifndef QUESTIONDIALOGPAGE_H__
#define QUESTIONDIALOGPAGE_H__

//#include "Pages.h"
#include "GUI.h"

class QuestionDialogPage : public LightPage {
public:
  char *title;
  char *text;
  QuestionDialogPage() : LightPage() { }

  bool handleEvent(gui_event_t *event);
  void init(const char* title_, const char* text_);
  void display();
  void setup() {}

  bool return_state = false;
};

extern QuestionDialogPage questiondialog_page;

#endif /* QUESTIONDIALOGPAGE_H__ */
