/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef EXTSEQTRACK_H__
#define EXTSEQTRACK_H__
#include "GUI.h"

class ExtSeqTrack : public SeqTrack {

public:
  ExtSeqTrack(Encoder *e1 = NULL,
               Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL)
      : SeqTrack(e1, e2, e3, e4) {}
  bool handleEvent(gui_event_t *event);
  bool displayPage();
  void setup();
};

#endif /* EXTSEQTRACK_H__ */
