#include "BrowserPage.hh"

BrowserPage::BrowserPage() {
  //  SDCard.init();
  numEntries = 0;
  posEncoder.initRangeEncoder(0, 0, "POS");
  setEncoders(&posEncoder);
  //  gotoDirectory("/");
}

void BrowserPage::display() {
  if (redisplay) {
    GUI.setLine(GUI.LINE1);
    GUI.put_value(0, posEncoder.getValue() + 1);
    GUI.put_string_at(3, "/");
    GUI.put_value_at(4, numEntries);

    GUI.setLine(GUI.LINE2);
    GUI.put_string_at_fill(0, entries[posEncoder.getValue()].name);
  }
}

void BrowserPage::loop() {
  if (posEncoder.hasChanged()) {
    redisplay = true;
  }
}

void BrowserPage::gotoDirectory(const char *path) {
  curDir.setPath(path);
  numEntries = curDir.listDirectory(entries, countof(entries));
  if (numEntries <= 0) {
    posEncoder.max = 0;
    numEntries = 0;
  } else {
    posEncoder.max = numEntries - 1;
    redisplay = true;
    posEncoder.setValue(0);
  }
}
