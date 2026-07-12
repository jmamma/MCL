#pragma once

#ifdef PLATFORM_TBD

#include "GUI.h"
#include "GUI/MCLEncoder.h"
#include <stddef.h>

class TbdTempoPage : public LightPage {
public:
  TbdTempoPage();

  void begin(bool tap);
  bool is_active() const { return active_; }

  virtual void init() override;
  virtual void cleanup() override;
  virtual void loop() override;
  virtual void display() override;
  virtual bool handleEvent(gui_event_t *event) override;

private:
  static constexpr uint16_t kMinTempoTenths = 300;
  static constexpr uint16_t kMaxTempoTenths = 3000;
  static constexpr uint8_t kMaxTapIntervals = 15;

  void close();
  bool tempo_edit_allowed() const;
  void sync_from_clock();
  void set_tempo_tenths(int16_t tenths);
  void adjust_tempo(int16_t delta_tenths);
  void reset_taps();
  void handle_tap();
  uint16_t calculate_tap_tempo_tenths() const;
  void draw_title(const char *title);
  void draw_tempo_value(uint16_t tempo_tenths, uint8_t y);
  void format_tempo(char *dst, size_t dst_len, uint16_t tempo_tenths) const;
  const char *clock_source_label() const;

  MCLRelativeEncoder tempo_encoder_;
  bool active_ = false;
  bool tap_mode_ = false;
  bool tap_on_open_ = false;
  uint16_t tempo_tenths_ = 1200;
  uint8_t tap_count_ = 0;
  uint8_t tap_index_ = 0;
  uint16_t last_tap_ms_ = 0;
  uint16_t tap_intervals_ms_[kMaxTapIntervals];
};

extern TbdTempoPage tbd_tempo_page;

#endif // PLATFORM_TBD
