#ifndef MNM_MAGIC_PAGE_H__
#define MNM_MAGIC_PAGE_H__

#include "MNM.h"
#include "AutoEncoderPage.h"

class MagicMNMPage : public AutoEncoderPage<MNMEncoder> {
  public:
  uint8_t track;
  uint8_t params[4];

  virtual void show(); 
  virtual void setup();

  void setup(uint8_t param1, uint8_t param2, uint8_t param3, uint8_t param4);

  void setTrack(uint8_t _track);
  
  void setToCurrentTrack();
  virtual bool handleEvent(gui_event_t *event);
};

class MagicSwitchPage : 
public SwitchPage {
public:
  MagicMNMPage magicPages[4];
  Page *currentPage;
  bool selectPage;

  MagicSwitchPage() : 
  SwitchPage("MAGIC PAGE:", 
  &magicPages[0], &magicPages[1], &magicPages[2], &magicPages[3]) {
    currentPage = NULL;
    selectPage = false;
  }
  
  void setup();
  void setPage(Page *page);
  
  virtual bool handleEvent(gui_event_t *event);
  
  virtual void update();
  virtual void finalize();

  void setToCurrentTrack() ;
  
  virtual void display();
  virtual void show();
  virtual void hide();
};

#endif /* MNM_MAGIC_PAGE_H__ */
