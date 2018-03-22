/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef A4SEQTRACK_H__
#define A4SEQTRACK_H__


class A4SeqTrack : public ExtSeqTrack {

public:
  A4SeqTrack(Encoder *e1 = NULL,
               Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL)
      : ExtSeqTrack(e1, e2, e3, e4) {}
  bool handleEvent(gui_event_t *event);
  void display();
  void setup();
};

#endif /* A4SEQTRACK_H__ */
