/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef PageSelectPAGE_H__
#define PageSelectPAGE_H__

#include "GUI.h"
#include "MCLSeq.h"

extern MCLEncoder page_select_param1;
extern MCLEncoder page_select_param2;

struct PageCategory {
  char Name[8];
  uint8_t PageCount;
  uint8_t FirstPage;
};

struct PageSelectEntry {
  char Name[16];
  LightPage *Page;
  uint8_t PageNumber; // same as trig id
  uint8_t CategoryId;
  uint8_t IconWidth;
  uint8_t IconHeight;
  uint8_t *IconData;
};

class MDCallback : public SysexCallback {
public:
  bool state = false;
  void init() { state = true; }
  void onReceived() {
    if (MD.kit.fromSysex(MD.midi)) {
      mcl_seq.update_kit_params();
    }
    auto listener = MD.getSysexListener();
    listener->removeOnKitMessageCallback(this);
    state = false;
  }
};

class PageSelectPage : public LightPage {
public:

  MDCallback kit_cb;

  bool loop_init = false;
  uint8_t page_select;
  PageSelectPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
                 Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {
    encoders[0] = e1;
    encoders[1] = e2;
  }

  virtual void display();
  virtual void setup();
  virtual void init();
  virtual void loop();
  virtual void cleanup();
  virtual void md_prepare();

  virtual bool handleEvent(gui_event_t *event);

  // get a page in the current category.
  uint8_t get_category_page(uint8_t offset);
  uint8_t get_nextpage_down();
  uint8_t get_nextpage_up();
  uint8_t get_nextpage_catup();
  uint8_t get_nextpage_catdown();
};

extern PageSelectPage page_select_page;
extern uint16_t trigled_mask;

#endif /* PageSelectPAGE_H__ */
