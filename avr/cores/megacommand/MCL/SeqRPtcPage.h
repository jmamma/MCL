/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQRPTCPAGE_H__
#define SEQRPTCPAGE_H__
#include "GUI.h"

class SeqRPtcPage : public SeqPtcPage {

public:
  SeqRPtcPage(Encoder *e1 = NULL,
               Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL)
      : SeqPtcPage(e1, e2, e3, e4) {}
  bool handleEvent(gui_event_t *event);
  bool display();
  void setup();
};

#endif /* SEQRPTCPAGE_H__ */
