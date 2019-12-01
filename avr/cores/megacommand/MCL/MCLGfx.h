/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MCLGFX_H__
#define MCLGFX_H__


class MCLGfx {
  public:
  void splashscreen();
  void init_oled();
  void display_text(const char *str1, const char *str2);
  void alert(const char *str1, const char *str2);

};

extern MCLGfx gfx;

#endif /* MCLGFX_H__ */
