#ifndef SPEEDSEQ_H__
#define SPEEDSEQ_H__

#include "WProgram.h"

// update on 32th notes

class SpeedSeq {
 public:
  SpeedSeq() {
    speed = 0x100;
    current_pos = 0;
    trigs = 0;
    offset = 0;
    
  }
  uint16_t speed;
  uint8_t trigs;
  uint16_t current_pos;
  
  int8_t offset;

  bool updatePos(void) {
    uint8_t old_value = current_pos >> 8;
    current_pos += speed;
    uint8_t new_value = current_pos >> 8;
    return (old_value != new_value);
  }
  
  uint8_t getCurrentStep() {
    return current_pos >> 8;
  }
  
  void setTrig(uint8_t step) {
    SET_BIT(trigs, step);
  }

  void clearTrig(uint8_t step) {
    CLEAR_BIT(trigs, step);
  }

  void toggleTrig(uint8_t step) {
    TOGGLE_BIT(trigs, step);
  }

  bool isHit(uint8_t step) {
    return IS_BIT_SET(trigs, step);
  }
};

#endif /* SPEEDSEQ_H__ */
