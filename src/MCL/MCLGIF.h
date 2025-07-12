#pragma once
#include "Arduino.h"

enum MCLGIFDir {
  DIR_FWD,
  DIR_FWDBACK,
};

class MCLGIF {
public:
  uint8_t frame_offset_bytes;
  uint8_t num_of_frames;
  uint8_t w;
  uint8_t h;
  MCLGIFDir dir;
  uint8_t loops;

  //Do not use constructor
  // MCLGIF(uint8_t b, uint8_t n, uint8_t w_, uint8_t h_, MCLGIFDir d, uint8_t l) : frame_offset_bytes(b), num_of_frames(n), w(w_), h(h_), dir(d),loops(l) { }

//private:

  uint8_t loop_count;
  int8_t cur_frame;
  int8_t inc;
  uint16_t last_frame_clock;
  uint8_t *bitmap;
  void reset() {
    if (inc == 0) {
      inc = 1;
      cur_frame = 0;
    }
    loop_count = 0;
  }

  void set_bmp(uint8_t *bmp) { bitmap = bmp; }

  uint8_t *get_frame(uint8_t n) {
    uint16_t index = frame_offset_bytes * n;
    return &bitmap[index];
  }

  uint8_t *get_current_frame() {
    uint8_t *bmp = get_frame(cur_frame);
    return bmp;
  }
  uint8_t *get_next_frame();
};


