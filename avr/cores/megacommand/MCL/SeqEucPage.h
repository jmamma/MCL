/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQEUCPAGE_H__
#define SEQEUCPAGE_H__
#include "GUI.h"

class SeqEucPage : public SeqPage {

public:
  SeqEucPage(Encoder *e1 = NULL,
               Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL)
      : SeqPage(e1, e2, e3, e4) {}
  bool handleEvent(gui_event_t *event);
  bool display();
  void setup();
};

#endif /* SEQEUCPAGE_H__ */
