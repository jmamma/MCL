/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQTRACK_H__
#define SEQTRACK_H__

class SeqTrack {

public:
  uint8_t track_type;
  uint8_t PatternLengths;

  SeqTrack(Encoder *e1 = NULL,
               Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL) {}
  void setup();
};

#endif /* SEQTRACK_H__ */
