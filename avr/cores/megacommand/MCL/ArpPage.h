/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef ARPPAGE_H__
#define ARPPAGE_H__

//#include "Pages.h"
#include "GUI.h"
#include "MCLEncoder.h"
#include "ArpSeqTrack.h"

extern MCLEncoder arp_rate;
extern MCLEncoder arp_oct;
extern MCLEncoder arp_mode;
extern MCLEncoder arp_enabled;

extern MCLEncoder arp_trig;
extern MCLEncoder arp_repeat;
extern MCLEncoder arp_gate;
extern MCLEncoder arp_steps;

class ArpPage : public LightPage {
public:
  ArpSeqTrack *arp_track;
  bool sub_page;

  ArpPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
            Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {
      }
  
  bool handleEvent(gui_event_t *event);
  void track_update();

  void config_encoders();

  void loop();
  void display();
  void setup();
  void init();
  void cleanup();
};

#endif /* ARPPAGE_H__ */
