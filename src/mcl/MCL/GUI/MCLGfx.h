/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#pragma once

#include "helpers.h"
#include "oled.h"
#include "GUI/MCLGIF.h"

#include "TomThumb.h"
#include "Elektrothic.h"

class MCLGfx {
public:
  void draw_evil(unsigned char *bitmap);
  void splashscreen(unsigned char *bitmap);
  void alert(const char *str1, const char *str2);
  void alert_error(const char *str2);
  void alert_success(const char *str2);
};
extern MCLGfx gfx;
