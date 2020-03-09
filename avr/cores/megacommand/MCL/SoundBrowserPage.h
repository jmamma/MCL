/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SOUNDBROWSERPAGE_H__
#define SOUNDBROWSERPAGE_H__

#include "FileBrowserPage.h"

class SoundBrowserPage : public FileBrowserPage {
  public:

  SoundBrowserPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
                  Encoder *e4 = NULL) : FileBrowserPage(e1, e2, e3, e4) {
  }

  uint8_t pending_action = 0;

  virtual void on_new();
  virtual void on_select(const char*);
  virtual void on_cancel();
  virtual bool handleEvent(gui_event_t *event);
  void draw_scrollbar(uint8_t x_offset);
  void init();
  void setup();
  void save_sound();
  void load_sound();
  void send_wav(int slot);
  void recv_wav(int slot);

};

extern SoundBrowserPage sound_browser;

#endif /* SOUNDBROWSERPAGE_H__ */
