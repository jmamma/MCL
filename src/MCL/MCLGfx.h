/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#pragma once

#include "Fonts/TomThumb.h"
#include "Fonts/Elektrothic.h"
#include "helpers.h"

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

  uint8_t *get_next_frame() {
    uint8_t *bmp = get_frame(cur_frame);
    if (clock_diff(last_frame_clock,g_clock_ms) < 80 || loop_count == loops) { return bmp; }
    last_frame_clock = g_clock_ms;
    cur_frame += inc;

    if (cur_frame < 0) {
      cur_frame = 0;
      inc = 1;
      loop_count++;
    }

    if (cur_frame == num_of_frames) {
      if (dir == DIR_FWD) {
        loop_count++;
        cur_frame = 0;
      }
      if (dir == DIR_FWDBACK) {
        cur_frame -= 2;
        inc = -1;
      }
    }
    return bmp;
  }
};

class MCLGfx {
public:
  void draw_evil(unsigned char *bitmap);
  void splashscreen(unsigned char *bitmap);
  void init_oled();
  void alert(const char *str1, const char *str2);
};
extern MCLGfx gfx;

