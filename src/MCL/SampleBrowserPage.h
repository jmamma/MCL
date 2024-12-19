/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SAMPLEBROWSERPAGE_H__
#define SAMPLEBROWSERPAGE_H__

#include "FileBrowserPage.h"
#include "MidiSysex.h"

#define FT_SND 0
#define FT_WAV 1
#define FT_SYX 2

#define PA_NEW 0
#define PA_SELECT 1

class SampleBrowserPage : public FileBrowserPage, public MidiSysexListenerClass {
  public:

  SampleBrowserPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
                  Encoder *e4 = NULL) : FileBrowserPage(e1, e2, e3, e4), MidiSysexListenerClass(){
    ids[0] = 0;
    ids[1] = 0x20;
    ids[2] = 0x3c;
  }

  uint8_t pending_action = 0;
  bool show_ram_slots = false;

  uint8_t old_cur_row = 255;

  virtual void on_new();
  virtual void on_select(const char*);
  virtual void on_cancel();
  virtual bool handleEvent(gui_event_t *event);
  virtual void display();

  void init(uint8_t show_samplemgr_);
  virtual void init() { init(false); }
  void setup();
  void send_sample(int slot, char *newname = nullptr, bool silent = false);
  void recv_wav(int slot, bool silent = false);

  virtual void start();
  virtual void end();
  virtual bool _handle_filemenu();
  protected:
  void query_sample_slots();
};

extern MCLEncoder samplebrowser_param1;
extern MCLEncoder samplebrowser_param2;
extern MCLEncoder samplebrowser_param3;
extern SampleBrowserPage sample_browser;

#endif /* SAMPLEBROWSERPAGE_H__ */
