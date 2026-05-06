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
    uint8_t driver_param = 255;
  };

  bool is_active() const { return latched_; }
  bool is_collapsed() const;
  uint8_t device_idx() const { return device_idx_; }
  uint8_t sub_page() const { return sub_page_; }
  uint8_t active_track_index() const;

  bool enter(uint8_t device_idx);
  void disable();
  bool handle_event(gui_event_t *event);
  void handle_ui_slot_button(bool pressed);
  void poll_encoders();

  TbdP4SoundData *active_sound() const;
  uint8_t window_count() const;
  bool param_slot(uint8_t window, uint8_t encoder_idx,
                  ParamSlot &slot) const;
  bool active_step_lock(uint8_t window, uint8_t encoder_idx,
                        uint8_t *value) const;
  bool show_strip_value(uint8_t encoder_idx) const;
  bool is_preset_page(uint8_t window) const;
  void render_preset_window(uint8_t y_top, bool active, uint8_t row_height);

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
  uint16_t ui_button_press_ms_ = 0;
  bool ui_button_pressed_ = false;
  bool ui_button_hold_handled_ = false;

  struct PresetGroup {
    char id[TBD_P4_ID_LEN];
    char name[TBD_P4_ID_LEN];
    uint8_t first_preset = 0;
    uint8_t preset_count = 0;
  };

  struct PresetEntry {
    char id[TBD_P4_ID_LEN];
    char name[TBD_P4_ID_LEN];
    uint8_t group = 0;
  };

  static constexpr uint8_t MAX_PRESET_GROUPS = 16;
  static constexpr uint8_t MAX_PRESETS = 64;

  PresetGroup preset_groups_[MAX_PRESET_GROUPS];
  PresetEntry presets_[MAX_PRESETS];
  uint8_t preset_group_count_ = 0;
  uint8_t preset_count_ = 0;
  uint8_t preset_cache_p4_track_ = 255;
  bool preset_cache_valid_ = false;
  bool preset_cache_failed_ = false;
  uint8_t selected_group_ = 0;
  uint8_t selected_preset_ = 0;
  bool preset_apply_in_progress_ = false;
  bool preset_apply_failed_ = false;

  void show_fullscreen();
  void show_strip();
  void poll_ui_button_hold();
  void resync_from_sound();
  void move_sub_page(int8_t delta);
  void select_sub_page_half(bool lower_half);
  bool encoder_passthrough_page() const;
  void send_param(uint8_t encoder_idx);
  bool write_step_locks(const ParamSlot &slot, uint8_t value);
  bool ensure_preset_cache();
  bool parse_preset_list_json(const char *json);
  void clear_preset_cache();
  void sync_preset_selection_to_sound();
  uint8_t selected_group_preset_count() const;
  uint8_t selected_global_preset_index() const;
  void select_global_preset(uint8_t preset_index);
  PresetEntry *selected_preset_entry();
  const PresetEntry *selected_preset_entry() const;
  void poll_preset_encoders(encoder_t *snapshot, uint16_t now);
  bool apply_selected_preset();
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
