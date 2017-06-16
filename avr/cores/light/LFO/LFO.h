#ifndef LFO_H__
#define LFO_H__

#include <inttypes.h>

typedef enum {
  LFO_NONE = 0,
  LFO_TRIANGLE,
  LFO_SQUARE,
  LFO_RANDOM
} lfo_type_t;

class LFO {
 protected:
  uint8_t oldacc;
 public:
  LFO();

  lfo_type_t type;
  
  uint16_t acc;
  uint32_t inc;

  uint8_t curValue;
  uint8_t oldValue;
  uint8_t amount;
  
  uint8_t getValue();
  uint8_t getScaledValue() {
    int val = getValue();
    return (val * (int)amount) >> 8;
  }

  void setSpeed(uint16_t _inc) { inc = _inc; }
  void tick() {
    acc += inc;
  }
  void update() {
    oldacc = acc;
  }
  

  bool hasChanged() {
    if (oldacc != acc) {
      return true;
    }
  }

};

#endif /* LFO_H__ */
