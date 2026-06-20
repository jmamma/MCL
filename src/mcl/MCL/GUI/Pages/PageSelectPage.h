/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef PageSelectPAGE_H__
#define PageSelectPAGE_H__

#include "GUI.h"
#include "GUI/MCLEncoder.h"
#include "MenuTypes.h"

extern MCLRelativeEncoder page_select_param1;
extern MCLRelativeEncoder page_select_param2;

class MidiDevice;

class PageSelectPage : public LightPage {
public:

  uint8_t page_select;
  PageSelectEntry page_entries[16];
  MidiDevice *page_select_ui_device;
  PageSelectPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
                 Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {
    encoders[0] = e1;
    encoders[1] = e2;
  }

  virtual void display();
  virtual void init();
  virtual void loop();
  virtual void cleanup();
  virtual void md_prepare();
  void prepare_overlay() { md_prepare(); }
  void draw_popup();
  virtual bool handleEvent(gui_event_t *event);

  // get a page in the current category.
  uint8_t category_page_in(uint8_t cat_id, uint8_t offset) const NOINLINE();
  uint8_t get_category_page(uint8_t offset);
  void rebuild_entries();
  PageIndex get_page(uint8_t page_number, char *str) const;

  // Mirror of the BUTTON2-release branch in handleEvent: navigate to the
  // currently-highlighted page (or back to GRID_PAGE if BUTTON1 is held or
  // the slot is empty). Used by the TBD ENC1-click toggle.
  void close_to_selection();
};

extern PageSelectPage page_select_page;
extern uint16_t trigled_mask;

#endif /* PageSelectPAGE_H__ */
