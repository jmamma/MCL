/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef LOUDNESSPAGE_H__
#define LOUDNESSPAGE_H__

//#include "Pages.h"
#include "MCLEncoder.h"
#include "GUI.h"

class LoudnessPage : public LightPage {
public:
  uint8_t last_midi_packet = 255;
  LoudnessPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
            Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {
      }

  bool handleEvent(gui_event_t *event);
  void display();
  void setup();
  void init();
  void cleanup();
  void scale_vol(float inc);
};

extern MCLEncoder loudness_param1;
extern LoudnessPage loudness_page;

#endif /* LOUDNESSPAGE_H__ */
