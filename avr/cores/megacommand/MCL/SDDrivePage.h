/* Yatao Li yatao.li@live.com 2019 */

#ifndef SDDRIVEPAGE_H__
#define SDDRIVEPAGE_H__

#include "FileBrowserPage.h"

class SDDrivePage : public FileBrowserPage {
public:

  SDDrivePage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
            Encoder *e4 = NULL)
      : FileBrowserPage(e1, e2, e3, e4) { }

  uint8_t progress_i;
  uint8_t progress_max;
  void init();
  void setup();
  void save_snapshot();
  void load_snapshot();
  virtual void on_select(const char*);
  virtual void on_new();
  virtual void display();
};

extern SDDrivePage sddrive_page;

#endif /* SDDRIVEPAGE_H__ */
