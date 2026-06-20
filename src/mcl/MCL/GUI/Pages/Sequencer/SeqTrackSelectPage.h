#pragma once

#ifdef PLATFORM_TBD

#include "GUI.h"
#include "GUI/MCLEncoder.h"
#include "../../../../Drivers/DeviceContext.h"

class SeqTrackSelectPage : public LightPage {
public:
  SeqTrackSelectPage();

  void begin();
  void end();
  bool is_active() const;
  void toggle_device();
  void move_track(int16_t delta);
  bool select_track(uint8_t track);

  virtual void init() override;
  virtual void cleanup() override;
  virtual void loop() override;
  virtual void display() override;

private:
  void sync_from_seq_page();
  void update_leds() const;
  bool select_track_via_tbd_ui(uint8_t track);
  uint8_t track_count() const;
  uint8_t current_track() const;

  MCLRelativeEncoder track_encoder_;
  bool active_ = false;
  DeviceIdx device_idx_ = DeviceIdx::Primary;
  uint8_t track_ = 0;
};

extern SeqTrackSelectPage seq_track_select_page;

#endif // PLATFORM_TBD
