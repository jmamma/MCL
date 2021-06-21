/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MCLGUI_H__
#define MCLGUI_H__

#include "QuestionDialogPage.h"
#include "TextInputPage.h"

#define SHOW_VALUE_TIMEOUT 2000

class MCLGUI {
public:
  uint8_t s_progress_cookie = 0b00110011;
  uint8_t s_progress_count = 0;

  void put_value_at(uint8_t value, char *str);

  void draw_textbox(const char *text, const char *text2);
  bool wait_for_input(char *dst, const char *title, uint8_t len);
  void draw_vertical_dashline(uint8_t x, uint8_t from = 1, uint8_t to = 32);
  void draw_horizontal_dashline(uint8_t y, uint8_t from, uint8_t to);
  void draw_horizontal_arrow(uint8_t x, uint8_t y, uint8_t w);
  bool wait_for_confirm(const char *title, const char *text);
  void draw_infobox(const char *line1, const char *line2,
                    const int line2_offset = 0);
  void draw_vertical_separator(uint8_t x);
  void draw_vertical_scrollbar(uint8_t x, uint8_t n_items, uint8_t n_window,
                               uint8_t n_current);
  ///  Clear the content area of a popup
  void clear_popup(uint8_t h = 0);
  void draw_popup(const char *title, bool deferred_display = false,
                  uint8_t h = 0);

  void draw_progress_bar(uint8_t cur, uint8_t _max,
                         bool deferred_display = true,
                         uint8_t x_pos = s_progress_x,
                         uint8_t y_pos = s_progress_y, uint8_t width = s_progress_w, uint8_t height = s_progress_h, bool border = true);

  void draw_progress(const char *msg, uint8_t cur, uint8_t _max,
                     bool deferred_display = false,
                     uint8_t x_pos = s_progress_x,
                     uint8_t y_pos = s_progress_y);

  void delay_progress(uint16_t clock_);

  void draw_microtiming(uint8_t resolution, uint8_t timing);
  void clear_leftpane();
  void clear_rightpane();

  void draw_encoder(uint8_t x, uint8_t y, uint8_t value);
  void draw_encoder(uint8_t x, uint8_t y, Encoder *encoder);

  bool show_encoder_value(Encoder *encoder);

  void draw_text_encoder(uint8_t x, uint8_t y, const char *name,
                         const char *value);
  void draw_md_encoder(uint8_t x, uint8_t y, Encoder *encoder,
                       const char *name);
  void draw_md_encoder(uint8_t x, uint8_t y, uint8_t value, const char *name,
                       bool show_value);
  void draw_light_encoder(uint8_t x, uint8_t y, Encoder *encoder,
                          const char *name);
  void draw_light_encoder(uint8_t x, uint8_t y, uint8_t value, const char *name,
                          bool show_value);
  void draw_keyboard(uint8_t x, uint8_t y, uint8_t note_width,
                     uint8_t note_height, uint8_t num_of_notes,
                     uint64_t *note_mask);
  void draw_trigs(uint8_t x, uint8_t y, uint8_t offset, const uint64_t &pattern_mask,
                  uint8_t step_count, uint8_t length, const uint64_t &mute_mask, const uint64_t &slide_mask);
  void draw_leds(uint8_t x, uint8_t y, uint8_t offset, const uint64_t &lock_mask,
                 uint8_t step_count, uint8_t length, bool show_current_step);

  void draw_track_type_select(uint8_t x, uint8_t y, uint8_t track_type_select);

  void draw_panel_toggle(const char *s1, const char *s2, bool s1_active);
  void draw_panel_labels(const char *info1, const char *info2);
  void draw_panel_status(bool recording, bool playing);
  void draw_panel_number(uint8_t number);

  void draw_knob_frame();
  void draw_knob(uint8_t i, const char *title, const char *text);
  void draw_knob(uint8_t i, Encoder *enc, const char *name);

  void init_encoders_used_clock() {

    for (uint8_t n = 0; n < GUI_NUM_ENCODERS; n++) {
      ((LightPage *)GUI.currentPage())->encoders_used_clock[n] =
          slowclock - SHOW_VALUE_TIMEOUT - 1;
    }
  }
  static constexpr uint8_t seq_w = 5;
  static constexpr uint8_t led_h = 3;
  static constexpr uint8_t trig_h = 5;

  static constexpr uint8_t s_menu_w = 104;
  static constexpr uint8_t s_menu_h = 24;
  static constexpr uint8_t s_menu_x = (128 - s_menu_w) / 2;
  static constexpr uint8_t s_menu_y = (32 - s_menu_h) / 2;
  static constexpr uint8_t s_title_x = 31;
  static constexpr uint8_t s_title_w = 64;

  static constexpr uint8_t s_rightpane_offset_x = 43;
  static constexpr uint8_t s_rightpane_offset_y = 8;

  static constexpr auto dlg_info_y1 = 2;
  static constexpr auto dlg_info_y2 = 27;
  static constexpr auto dlg_info_x1 = 12;
  static constexpr auto dlg_info_x2 = 124;
  static constexpr auto dlg_circle_x = dlg_info_x1 + 10;
  static constexpr auto dlg_circle_y = dlg_info_y1 + 15;

  static constexpr auto dlg_info_w = dlg_info_x2 - dlg_info_x1 + 1;
  static constexpr auto dlg_info_h = dlg_info_y2 - dlg_info_y1 + 1;

  static constexpr uint8_t pane_label_x = 0;
  static constexpr uint8_t pane_label_md_y = 0;
  static constexpr uint8_t pane_label_ex_y = 7;
  static constexpr uint8_t pane_label_w = 13;
  static constexpr uint8_t pane_label_h = 7;

  static constexpr uint8_t pane_info1_y = 19;
  static constexpr uint8_t pane_info2_y = 26;
  static constexpr uint8_t pane_info_h = 7;

  static constexpr uint8_t pane_x1 = 0;
  static constexpr uint8_t pane_x2 = 30;
  static constexpr uint8_t pane_w = pane_x2 - pane_x1;

  static constexpr uint8_t pane_cir_x1 = 22;
  static constexpr uint8_t pane_cir_x2 = 25;
  static constexpr uint8_t pane_tri_x = 16;
  static constexpr uint8_t pane_tri_y = 9;

  static constexpr uint8_t pane_trackid_x = 15;
  static constexpr uint8_t pane_trackid_y = 8;

  static constexpr uint8_t seq_x0 = 32;
  static constexpr uint8_t led_y = 22;
  static constexpr uint8_t trig_y = 26;

  static constexpr uint8_t knob_x0 = 31;
  static constexpr uint8_t knob_w = 24;
  static constexpr uint8_t knob_xend = 127;
  static constexpr uint8_t knob_y0 = 1;
  static constexpr uint8_t knob_y = 20;

  static constexpr uint8_t s_progress_x = 31;
  static constexpr uint8_t s_progress_y = 16;
  static constexpr uint8_t s_progress_w = 64;
  static constexpr uint8_t s_progress_h = 5;

  static constexpr uint8_t s_progress_speed = 2;
};

extern MCLGUI mcl_gui;
/*
// 'encoder_medium_1', 15x15px
const unsigned char encoder_medium_0 [] PROGMEM = {
        0x07, 0xc0, 0x18, 0x30, 0x20, 0x08, 0x40, 0x04, 0x40, 0x04, 0x80, 0x02,
0x80, 0x02, 0x80, 0x02, 0x80, 0x02, 0x80, 0x02, 0x40, 0x04, 0x43, 0x84, 0x23,
0x88, 0x18, 0x30, 0x07, 0xc0
};
// 'encoder_medium_2', 15x15px
const unsigned char encoder_medium_1 [] PROGMEM = {
        0x07, 0xc0, 0x18, 0x30, 0x20, 0x08, 0x40, 0x04, 0x40, 0x04, 0x80, 0x02,
0x80, 0x02, 0x80, 0x02, 0x80, 0x02, 0x80, 0x02, 0x40, 0x04, 0x47, 0x04, 0x23,
0x08, 0x18, 0x30, 0x07, 0xc0
};
// 'encoder_medium_3', 15x15px
const unsigned char encoder_medium_2 [] PROGMEM = {
        0x07, 0xc0, 0x18, 0x30, 0x20, 0x08, 0x40, 0x04, 0x40, 0x04, 0x80, 0x02,
0x80, 0x02, 0x80, 0x02, 0x80, 0x02, 0x80, 0x02, 0x48, 0x04, 0x46, 0x04, 0x26,
0x08, 0x18, 0x30, 0x07, 0xc0
};
// 'encoder_medium_4', 15x15px
const unsigned char encoder_medium_3 [] PROGMEM = {
        0x07, 0xc0, 0x18, 0x30, 0x20, 0x08, 0x40, 0x04, 0x40, 0x04, 0x80, 0x02,
0x80, 0x02, 0x80, 0x02, 0x80, 0x02, 0x90, 0x02, 0x58, 0x04, 0x4c, 0x04, 0x20,
0x08, 0x18, 0x30, 0x07, 0xc0
};
// 'encoder_medium_5', 15x15px
const unsigned char encoder_medium_4 [] PROGMEM = {
        0x07, 0xc0, 0x18, 0x30, 0x20, 0x08, 0x40, 0x04, 0x40, 0x04, 0x80, 0x02,
0x80, 0x02, 0x80, 0x02, 0xb0, 0x02, 0xb0, 0x02, 0x50, 0x04, 0x40, 0x04, 0x20,
0x08, 0x18, 0x30, 0x07, 0xc0
};
// 'encoder_medium_6', 15x15px
const unsigned char encoder_medium_5 [] PROGMEM = {
        0x07, 0xc0, 0x18, 0x30, 0x20, 0x08, 0x40, 0x04, 0x40, 0x04, 0x80, 0x02,
0x80, 0x02, 0xa0, 0x02, 0xb0, 0x02, 0xb0, 0x02, 0x40, 0x04, 0x40, 0x04, 0x20,
0x08, 0x18, 0x30, 0x07, 0xc0
};
*/
// 'encoder0', 11x11px
extern const unsigned char encoder_small_0[];
// 'encoder1', 11x11px
extern const unsigned char encoder_small_1[];
// 'encoder2', 11x11px
extern const unsigned char encoder_small_2[];
// 'encoder3', 11x11px
extern const unsigned char encoder_small_3[];
// 'encoder4', 11x11px
extern const unsigned char encoder_small_4[];
// 'encoder5', 11x11px
extern const unsigned char encoder_small_5[];
// 'encoder6', 11x11px
extern const unsigned char encoder_small_6[];

// 'wheel1', 19x19px
extern const unsigned char wheel_top [];
// 'wheel2', 19x19px
extern const unsigned char wheel_angle [];
// 'wheel3', 19x19px
extern const unsigned char wheel_side [];

#endif /* MCLGUI_H__ */
