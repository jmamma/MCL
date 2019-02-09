/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MCLGUI_H__
#define MCLGUI_H__

#include "TextInputPage.h"

class MCLGUI {
public:
  bool wait_for_input(char *dst, char *title, uint8_t len);
};

extern MCLGUI mcl_gui;

#endif /* MCLGUI_H__ */
