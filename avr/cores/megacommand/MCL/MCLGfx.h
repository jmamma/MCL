/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MCLGFX_H__
#define MCLGFX_H__


class MCLGfx {
  public:
  void draw_evil(unsigned char* bitmap);
  void splashscreen(unsigned char* bitmap);
  void init_oled();
  void display_text(const char *str1, const char *str2);
  void alert(const char *str1, const char *str2);

};
extern MCLGfx gfx;

#endif /* MCLGFX_H__ */
