/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQWRITEPAGE_H__
#define SEQWRITEPAGE_H__

#include "GUI.h"

class SeqWritePage : FlexPage {
 public:
 SeqWritePage(void (*func_point)(uint8_t), Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL) : FlexPage(func_point, e1, e2, e3 ,e4) {

 }
 bool handleEvent(gui_event_t *event);
 bool displayPage();
};

#endif /* SEQWRITEPAGE_H__ */
