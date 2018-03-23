/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MCLSYSTEMPAGE_H__
#define MCLSYSTEMPAGE_H__


class MCLSystemPage : public LightPage {
 public:
 MCLSystemPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL) : LightPage( e1, e2, e3 ,e4) {

 }
 void display();
 virtual bool handleEvent(gui_event_t *event);
};

#endif /* MCLSYSTEMPAGE_H__ */
