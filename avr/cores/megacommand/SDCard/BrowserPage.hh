#ifndef BROWSERPAGE_H__
#define BROWSERPAGE_H__

#include "SDCard.h"
#include "GUI.h"

class BrowserPage : public EncoderPage {
public:
  RangeEncoder posEncoder;
  SDCardEntry curDir;
  SDCardEntry entries[64];
  int numEntries;

  BrowserPage();

  void display();

  void loop();

  void gotoDirectory(const char *path);
};

#endif /* BROWSERPAGE_H__ */
