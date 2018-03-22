/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQSTEPPAGE_H__
#define SEQSTEPPAGE_H__

#include "SeqPage.h"

class SeqStepPage : public SeqPage {

public:
  SeqStepPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
              Encoder *e4 = NULL)
      : SeqPage(e1, e2, e3, e4) {}
  bool handleEvent(gui_event_t *event);
  void display();
  void setup();
  void init();
};

#endif /* SEQSTEPPAGE_H__ */
