#pragma once

#include "platform.h"

#ifdef PLATFORM_TBD

#include "GUI.h"
#include "MCLEncoder.h"
#include "TbdP4SoundData.h"
#include <stddef.h>
#include <stdint.h>

class TbdUiMode {
public:
  static constexpr uint8_t SLOT_PRIMARY = 0;
  static constexpr uint8_t SLOT_SECONDARY = 1;
  static constexpr uint8_t SLOT_NONE = 255;

  struct ParamSlot {
    TbdP4SoundData *sound = nullptr;
    TbdP4ParamDescriptor *param = nullptr;
    uint8_t lock_param = 0;
  };

  bool is_active() const { return latched_; }
  uint8_t device_idx() const { return device_idx_; }
  uint8_t sub_page() const { return sub_page_; }
  uint8_t active_track_index() const;

  bool enter(uint8_t device_idx);
  void disable();
  bool handle_event(gui_event_t *event);
  void poll_encoders();

  TbdP4SoundData *active_sound() const;
  uint8_t window_count() const;
  bool param_slot(uint8_t window, uint8_t encoder_idx,
                  ParamSlot &slot) const;
  bool active_step_lock(uint8_t window, uint8_t encoder_idx,
                        uint8_t *value) const;
  bool show_strip_value(uint8_t encoder_idx) const;

  MCLEncoder enc[4] = {
      MCLEncoder(127, 0, 1, 4),
      MCLEncoder(127, 0, 1, 4),
      MCLEncoder(127, 0, 1, 4),
      MCLEncoder(127, 0, 1, 4),
  };

private:
  bool latched_ = false;
  uint8_t device_idx_ = SLOT_NONE;
  uint8_t sub_page_ = 0;
  uint8_t bound_device_idx_ = SLOT_NONE;
  uint8_t bound_track_ = 255;
  uint8_t bound_sub_page_ = 255;
  uint16_t enc_used_clock_[4] = {0, 0, 0, 0};

  void show_fullscreen();
  void show_strip();
  void resync_from_sound();
  void move_sub_page(int8_t delta);
  void flip_sub_page_half();
  bool encoder_passthrough_page() const;
  void send_param(uint8_t encoder_idx);
  bool write_step_locks(const ParamSlot &slot, uint8_t value);
};

class TbdParamStripPage : public LightPage {
public:
  TbdParamStripPage() : LightPage() {}

  virtual void setup() override {}
  virtual void init() override {}
  virtual void cleanup() override {}
  virtual void loop() override {}
  virtual void display() override;
};

class TbdParamOverlayPage : public LightPage {
public:
  TbdParamOverlayPage() : LightPage() {}

  virtual void setup() override {}
  virtual void init() override {}
  virtual void cleanup() override {}
  virtual void loop() override {}
  virtual void display() override;
};

extern TbdUiMode tbd_ui_mode;
extern TbdParamStripPage tbd_param_strip_page;
extern TbdParamOverlayPage tbd_param_overlay_page;

#endif // PLATFORM_TBD
