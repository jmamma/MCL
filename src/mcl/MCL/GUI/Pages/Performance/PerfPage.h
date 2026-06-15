/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef PERFPAGE_H__
#define PERFPAGE_H__

#include "GUI.h"
#include "MCLEncoder.h"

#include "GUI/Pages/Performance/PerfPageParent.h"
#include "GUI/Pages/Performance/PerfEncoder.h"

#define NUM_PERF_CONTROLS 4
#define PERF_DESTINATION 0

class PerfPage : public LightPage, PerfPageParent {
public:
  PerfPage(Encoder *e1 = NULL, Encoder *e2 = NULL,
          Encoder *e3 = NULL, Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {
  }

  uint8_t undo = 255;

  uint8_t learn;
  uint8_t last_page_mode;

  uint8_t page_mode;
  uint8_t perf_id;

  uint16_t last_mask;
  uint16_t last_blink_mask;

  bool show_menu;

  PerfEncoder *perf_encoders[4];
  int8_t lfo_mod_delta[NUM_PERF_CONTROLS];
  uint8_t lfo_mod_dirty_mask;

  bool handleEvent(gui_event_t *event) override;
#if defined(MCL_HAS_DESKTOP_MOUSE)
  bool handleMouseEvent(mcl_mouse_event_t *event) override;
#endif
  void update_params();
  void learn_param(uint8_t dest, uint8_t param, uint8_t value);

  void display() override;
  void setup() override;

  void init() override;
  void loop() override;
  void cleanup() override;

  void func_enc_check();
  void set_led_mask();
  void config_encoders(uint8_t show_val = false);

  void send_locks(uint8_t mode);

  void config_encoder_range();

  void encoder_check();
  void encoder_send();
  void set_lfo_mod(uint8_t perf_idx, int8_t delta);
  void send_perf_encoder(uint8_t perf_idx, MidiUartClass *uart_ = nullptr,
                         MidiUartClass *uart2_ = nullptr);
};
extern void rename_perf();

extern MCLEncoder perf_page_param1;
extern MCLEncoder perf_page_param2;
extern MCLEncoder perf_page_param3;
extern MCLEncoder perf_page_param4;

#endif /* PERFPAGE_H__ */
