/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MCLGUI_H__
#define MCLGUI_H__

#include "QuestionDialogPage.h"
#include "TextInputPage.h"
#include "KeyInterface.h"
#include "NoteInterface.h"
#include "MCLMenus.h"
#include "LED.h"

//class MCLGUI : public Print {
class MCLGUI {
public:

  /*
   * Print child implementation:
   *
  char str_print[30];
  bool str_offset = 0;
  size_t write(uint8_t c) { };
  size_t write(const uint8_t *buffer, size_t size) {
    uint8_t s =  min(sizeof(str_print),size);
    strncpy(str_print, buffer + str_offset, s);
    str_offset += s;
  }

  void print_str() { oled_display.print(str_print); str_offset = 0; }
  */

  uint8_t s_progress_cookie = 0b00110011;
  uint8_t s_progress_count = 0;

  void put_value_at(uint8_t value, char *str, bool fixed = false);
  void put_value_at2(uint8_t value, char *str) { put_value_at(value, str, true); }

  void wait_for_project();
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
  void clear_popup(uint8_t h = 0, uint8_t y_offset = 0);
  void draw_popup_title(const char *title, uint8_t y_offset = 0);
  void draw_popup_title_plain(const char *title, uint8_t y_offset = 0);
  void draw_popup(const char *title, bool deferred_display = false,
                  uint8_t h = 0, uint8_t y_offset = 0);
  void draw_trigs(uint8_t x, uint8_t y,
                        const uint16_t &trig_selection);
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
#if !defined(__AVR__)
  void draw_microtiming_spsx(uint8_t speed, int8_t microtiming);
#endif
  void clear_leftpane();
  void clear_rightpane();

  void draw_encoder(uint8_t x, uint8_t y, uint8_t value, bool highlight = false);
  void draw_encoder(uint8_t x, uint8_t y, Encoder *encoder);

  // Bottom-strip encoder readout (TBD only, free real estate at y=32..63
  // on the 128x64 panel). Layout per column: label on top, dial in
  // middle, value below (only when show_values[i] is true). Pass nullptr
  // for any unused encoder; nullptr labels print blank. Reuses
  // draw_md_encoder so the look matches the rest of the GUI.
  void draw_encoder_strip(uint8_t y_top, Encoder *const encoders[4],
                          const char *const labels[4],
                          const bool show_values[4]);

  bool show_encoder_value(Encoder *encoder, int timeout = SHOW_VALUE_TIMEOUT);

  void draw_cross(uint8_t x, uint8_t y, uint8_t color = WHITE);

  void draw_text_encoder(uint8_t x, uint8_t y, const char *name,
                         const char *value, bool highlight = false,
                         bool name_is_progmem = true);
  void draw_md_encoder(uint8_t x, uint8_t y, Encoder *encoder,
                       const char *name);
  void draw_md_encoder(uint8_t x, uint8_t y, uint8_t value, const char *name,
                       bool show_value, bool name_is_progmem = true);
  void draw_light_encoder(uint8_t x, uint8_t y, Encoder *encoder,
                          const char *name, bool highlight = false,
                          bool name_is_progmem = true);
  void draw_light_encoder(uint8_t x, uint8_t y, uint8_t value, const char *name,
                          bool highlight = false, bool show_value = false,
                          bool name_is_progmem = true);
  void draw_keyboard(uint8_t x, uint8_t y, uint8_t note_width,
                     uint8_t note_height, uint8_t num_of_notes,
                     uint64_t *note_mask);
  void draw_trigs(uint8_t x, uint8_t y, uint8_t offset, const uint64_t &pattern_mask,
                  uint8_t step_count, uint8_t length, const uint64_t &mute_mask, const uint64_t &slide_mask);
  void draw_leds(uint8_t x, uint8_t y, uint8_t offset, const uint64_t &lock_mask,
                 uint8_t step_count, uint8_t length, bool show_current_step);

  void draw_track_type_select(uint8_t track_type_select,
                              uint8_t y_offset = 0);

  void draw_panel_toggle(const char *s1, const char *s2, bool s1_active);
  void draw_panel_labels(const char *info1, const char *info2);
  void draw_panel_status(bool recording, bool playing);
  void draw_panel_number(uint8_t number);

  void draw_knob_frame();
  void draw_knob(uint8_t i, const char *title, const char *text,
                 bool title_is_progmem = true);
  void draw_knob(uint8_t i, Encoder *enc, const char *name,
                 bool highlight = false, bool title_is_progmem = true);

  void drawRoundRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);
  // Bank-popup cell: rounded rect with letter inside. Active = filled
  // interior with letter drawn inverse; inactive = outline only.
  void draw_bank_cell(uint8_t x, uint8_t y, uint8_t w, uint8_t h, char letter,
                      bool active);

  void set_trigleds(uint16_t bitmask, TrigLEDMode mode, bool blink = false);
  // Local-only — does not echo to the MD. For internal MCL modes that
  // shouldn't pilot the MD's display (e.g. the MCL_B bank-select stage).
  void set_trigleds_local(uint16_t bitmask, TrigLEDMode mode, bool blink = false);
  void set_trigleds_color(uint16_t bitmask, uint32_t rgb);
  void set_trigleds_blink_color(uint16_t bitmask, uint32_t rgb);
  void reset_trigleds();

  static constexpr uint8_t seq_w = 5;
  static constexpr uint8_t led_h = 3;
  static constexpr uint8_t trig_h = 5;

  static constexpr uint8_t s_menu_w = 104;
  static constexpr uint8_t s_menu_h = 27;
  static constexpr uint8_t s_menu_x = (128 - s_menu_w) / 2;
  static constexpr uint8_t s_menu_y = 0;
  static constexpr uint8_t s_title_x = 31;
  static constexpr uint8_t s_title_w = 64;

  static constexpr uint8_t s_rightpane_offset_x = 43;
  static constexpr uint8_t s_rightpane_offset_y = 8;

  static constexpr auto dlg_info_y1 = 1;
  static constexpr auto dlg_info_y2 = 28;
  static constexpr auto dlg_info_x1 = 8;
  static constexpr auto dlg_info_x2 = 120;
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

  static constexpr uint8_t s_progress_x = 32;
  static constexpr uint8_t s_progress_y = 16;
  static constexpr uint8_t s_progress_w = 64;
  static constexpr uint8_t s_progress_h = 5;

  static constexpr uint8_t s_progress_speed = 2;
};

extern MCLGUI mcl_gui;

// Helper function for printing PROGMEM strings
void mcl_print_P(const char* str_P);
void mcl_println_P(const char* str_P);

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
