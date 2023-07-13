/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MCLGFX_H__
#define MCLGFX_H__

enum MCLGIFDir {
  DIR_FWD,
  DIR_FWDBACK,
};

class MCLGIF {
private:
  int8_t cur_frame = 0;
  int8_t inc = 0;
  uint16_t last_frame_clock;
public:
  uint8_t *bitmap;
  uint8_t frame_offset_bytes;
  uint8_t num_of_frames;
  uint8_t w;
  uint8_t h;
  uint8_t loops;

  uint8_t loop_count;
  MCLGIFDir dir;

  MCLGIF(uint8_t frame_offset_, uint8_t num_of_frames_, uint8_t w_, uint8_t h_, MCLGIFDir dir_, uint8_t loops_ = 0) {
    frame_offset_bytes = frame_offset_;
    num_of_frames = num_of_frames_;
    w = w_;
    h = h_;
    dir = dir_;
    loops = loops_;
    reset();
    loop_count = loops;
  }

  void reset() {
    inc = 1;
    cur_frame = 0;
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
    if (clock_diff(last_frame_clock,slowclock) < 80 || loop_count == loops) { return bmp; }
    last_frame_clock = slowclock;
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

extern MCLGIF metronome_gif;
extern MCLGIF perf_gif;
extern MCLGIF route_gif;
extern MCLGIF analog_gif;
extern MCLGIF midi_gif;
extern MCLGIF monomachine_gif;
extern MCLGIF machinedrum_gif;
#endif /* MCLGFX_H__ */
