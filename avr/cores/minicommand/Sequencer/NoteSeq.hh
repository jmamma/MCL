#ifndef NOTESEQ_H__
#define NOTESEQ_H__

#define NO_NOTE 0xFF

class NoteSeq {
  public:
  NoteSeq() {
    len = 0;
    for (uint8_t i = 0; i < sizeof(notes); i++) {
      notes[i] = NO_NOTE;
    }
  }
  
  uint8_t notes[32];
  uint8_t len;
  
  bool isHit(uint8_t step) {
    return (notes[step] == NO_NOTE);
  }
};

#endif /* NOTESEQ_H__ */
