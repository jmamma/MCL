/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SOUNDBROWSERPAGE_H__
#define SOUNDBROWSERPAGE_H__

#include "FileBrowserPage.h"

#define PA_NEW 0
#define PA_SELECT 1

class SoundBrowserPage : public FileBrowserPage {
  public:

  SoundBrowserPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
                  Encoder *e4 = NULL) : FileBrowserPage(e1, e2, e3, e4) {
  }

  virtual void on_new();
  virtual void on_select(const char*);
  virtual void on_cancel();

  void save_sound();
  void load_sound();

  virtual bool handleEvent(gui_event_t *event);

  virtual void init();
  void setup();

};

extern SoundBrowserPage sound_browser;

#endif /* SOUNDBROWSERPAGE_H__ */
