/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MDSEQTRACK_H__
#define MDSEQTRACK_H__
#include "GUI.h"

class MDSeqTrack : public SeqTrack {

public:
  MDSeqTrack(Encoder *e1 = NULL,
               Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL)
      : SeqTrack(e1, e2, e3, e4) {}
  bool handleEvent(gui_event_t *event);
  bool display();
  void setup();
};

#endif /* MDSEQTRACK_H__ */
