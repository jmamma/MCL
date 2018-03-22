/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef NEWPROJECTPAGE_H__
#define NEWPROJECTPAGE_H__


char allowedchars;
class NewProjectPage : LightPage {
public:
  char newprj[18];

  NewProjectPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
                 Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {}
  virtual bool handleEvent(gui_event_t *event);
  virtual void display();
  void setup();
  void update_prjpage_char();
};

#endif /* NEWPROJECTPAGE_H__ */
