/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQRTRKPAGE_H__
#define SEQRTRKPAGE_H__
#include "GUI.h"

class SeqRtrkPage : public SeqPage {

public:
  SeqRtrkPage(Encoder *e1 = NULL,
               Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL)
      : SeqPage(e1, e2, e3, e4) {}
  bool handleEvent(gui_event_t *event);
  bool displayPage();
  void setup();
};

#endif /* SEQRTRKPAGE_H__ */
