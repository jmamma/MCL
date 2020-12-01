/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef CONVERTPROJECTPAGE_H__
#define CONVERTPROJECTPAGE_H__

#include "GUI.h"
#include "FileBrowserPage.h"

#define MAX_ENTRIES 1024
#ifdef OLED_DISPLAY
#define MAX_VISIBLE_ROWS 4
#else
#define MAX_VISIBLE_ROWS 1
#endif

#define MENU_WIDTH 78

class ConvertProjectPage : public FileBrowserPage {
public:

  ConvertProjectPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
                  Encoder *e4 = NULL)
      : FileBrowserPage(e1, e2, e3, e4) {}
  virtual void on_select(const char* entry);
  void init();
};

#endif /* CONVERTPROJECTPAGE_H__ */
