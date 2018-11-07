/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MCLGFX_H__
#define MCLGFX_H__


class MCLGfx {
  public:
  void splashscreen();
  void init_oled();
  void alert(char *str1, char *str2);
};

extern MCLGfx gfx;

#endif /* MCLGFX_H__ */
