/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef LFOPAGE_H__
#define LFOPAGE_H__

#include "GUI.h"
#include "MCLEncoder.h"
#include "LFO.h"
#include "LFOSeqTrack.h"
#include "LFOTrackRef.h"
#include "SeqPage.h"
#include "PerfPageParent.h"

#define NUM_LFO_PAGES 2


class LFOPage : public SeqPage, PerfPageParent {
public:
  LFOPage(LFOSeqTrack *lfo_track_, Encoder *e1 = NULL, Encoder *e2 = NULL,
          Encoder *e3 = NULL, Encoder *e4 = NULL)
      : SeqPage(e1, e2, e3, e4) {
  lfo_track = lfo_track_;
  }

  bool handleEvent(gui_event_t *event) override;
  bool midi_state;

  uint8_t page_mode;
  uint8_t page_id;
  LFOSeqTrack *lfo_track;

  uint8_t waveform;
  uint8_t depth;
  uint8_t depth2;

  void display() override;
  void setup() override;
//  void draw_pattern_mask();
  void init() override;
  void loop() override;
  void cleanup() override;
  virtual void config_encoders() override;
  virtual bool moveEncoderFocusPage(int8_t direction) override;

  void track_update();
  void config_encoder_range(uint8_t i);
  void learn_param(uint8_t track, uint8_t param, uint8_t value);
  void learn_perf_dest(uint8_t global_dest, uint8_t param, uint8_t value);
  void learn_param(DeviceIdx device_idx, uint8_t target, uint8_t param,
                   uint8_t value);

protected:
  virtual void capture_seq_menu_values(bool is_md_device) override;
  virtual void apply_seq_menu_values(bool same_slot) override;
  virtual bool apply_seq_menu_row(uint8_t row_entry,
                                  void (*row_func)()) override;

private:
  void select_menu_track(uint8_t track);
  bool refresh_track_selection();
  void sync_lfo_track();
  void clear_lfo_track();
  void copy_lfo_track();
  void paste_lfo_track();
};

extern MCLEncoder lfo_page_param1;
extern MCLEncoder lfo_page_param2;
extern MCLEncoder lfo_page_param3;
extern MCLEncoder lfo_page_param4;

#endif /* LFOPAGE_H__ */
