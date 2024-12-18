/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef LFOPAGE_H__
#define LFOPAGE_H__

#include "GUI.h"
#include "MCLEncoder.h"
#include "LFOSeqTrack.h"
#include "SeqPage.h"
#include "PerfPageParent.h"

#define NUM_LFO_PAGES 2

#define SIN_WAV 0
#define TRI_WAV 1
#define RAMP_WAV 2
#define IEXP_WAV 3
#define IRAMP_WAV 4
#define EXP_WAV 5


class LFOPage : public SeqPage, PerfPageParent {
public:
  LFOPage(LFOSeqTrack *lfo_track_, Encoder *e1 = NULL, Encoder *e2 = NULL,
          Encoder *e3 = NULL, Encoder *e4 = NULL)
      : SeqPage(e1, e2, e3, e4) {
  lfo_track = lfo_track_;
  }

  bool handleEvent(gui_event_t *event);
  bool midi_state = false;

  uint8_t page_mode;
  uint8_t page_id;
  LFOSeqTrack *lfo_track;

  uint8_t waveform;
  uint8_t depth;
  uint8_t depth2;

  void display();
  void setup();
//  void draw_pattern_mask();
  void init();
  void loop();
  void cleanup();
  virtual void config_encoders();

  void config_encoder_range(uint8_t i);
  void learn_param(uint8_t track, uint8_t param, uint8_t value);
};

extern MCLEncoder lfo_page_param1;
extern MCLEncoder lfo_page_param2;
extern MCLEncoder lfo_page_param3;
extern MCLEncoder lfo_page_param4;

#endif /* LFOPAGE_H__ */
