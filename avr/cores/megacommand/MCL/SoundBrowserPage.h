/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SOUNDBROWSERPAGE_H__
#define SOUNDBROWSERPAGE_H__

#include "FileBrowserPage.h"
#include "MidiSysex.h"

class SoundBrowserPage : public FileBrowserPage, public MidiSysexListenerClass {
  public:

  SoundBrowserPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
                  Encoder *e4 = NULL) : FileBrowserPage(e1, e2, e3, e4), MidiSysexListenerClass(){
    ids[0] = 0;
    ids[1] = 0x20;
    ids[2] = 0x3c;
  }

  uint8_t pending_action = 0;
  bool show_ram_slots = false;

  virtual void on_new();
  virtual void on_select(const char*);
  virtual void on_cancel();
  virtual bool handleEvent(gui_event_t *event);
  void draw_scrollbar(uint8_t x_offset);
  void init();
  void setup();
  void cleanup();
  void save_sound();
  void load_sound();
  void send_wav(int slot);
  void recv_wav(int slot);

  // MidiSysexListenerClass
  virtual void start();
  virtual void end();
  virtual void end_immediate();

protected:
  void query_sample_slots();
};

extern SoundBrowserPage sound_browser;

#endif /* SOUNDBROWSERPAGE_H__ */
