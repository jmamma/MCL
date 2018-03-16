/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQEUCPTCPAGE_H__
#define SEQEUCPTCPAGE_H__
#include "GUI.h"

class SeqEucPtcPage : public SeqPage {

public:
  SeqEucPtcPage(Encoder *e1 = NULL,
               Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL)
      : SeqPage(e1, e2, e3, e4) {}
  bool handleEvent(gui_event_t *event);
  bool display();
  void setup();
  void init();
};

#endif /* SEQEUCPTCPAGE_H__ */
