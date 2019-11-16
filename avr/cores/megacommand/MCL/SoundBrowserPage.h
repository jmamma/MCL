/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SOUNDBROWSERPAGE_H__
#define SOUNDBROWSERPAGE_H__

#include "FileBrowserPage.h"

class SoundBrowserPage : public FileBrowserPage {
  public:

  SoundBrowserPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
                  Encoder *e4 = NULL)
      : FileBrowserPage(e1, e2, e3, e4) {}
  virtual void on_new();
  virtual void on_select(const char*);
  virtual void on_cancel();
  void add_entry(char *entry);
  void draw_scrollbar(uint8_t x_offset);
  void init();
  void setup();
  void save_sound();
  void load_sound();
};

extern MCLEncoder soundbrowser_param1;
extern MCLEncoder soundbrowser_param2;
extern SoundBrowserPage sound_browser;

#endif /* SOUNDBROWSERPAGE_H__ */
