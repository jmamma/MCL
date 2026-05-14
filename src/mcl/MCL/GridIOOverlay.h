#pragma once

#ifdef PLATFORM_TBD

#include "GUI.h"
#include "MCLEncoder.h"
#include "MCLMemory.h"

class GridIOOverlay : public LightPage {
public:
  enum Mode : uint8_t {
    MODE_LOAD,
    MODE_SAVE,
  };

  GridIOOverlay();

  void begin(Mode mode);
  bool is_active() const;
  bool is_slot_selected(uint8_t visible_slot) const;

  virtual void init() override;
  virtual void cleanup() override;
  virtual void loop() override;
  virtual void display() override;
  virtual bool handleEvent(gui_event_t *event) override;

private:
  void close();
  void action();
  void group_action();
  void group_select();
  void toggle_grid();
  void focus_slot(GridSlot slot);
  void selected_tracks(uint8_t *track_select_array);
  uint16_t visible_select_mask() const;
  void load_mode_title(char *title, uint8_t size) const;
  void sync_preview_grid();

  Mode mode_ = MODE_LOAD;
  GridIndex saved_grid_ = 0;
  MCLEncoder mode_enc_;
  MCLExpEncoder queue_len_enc_;
  MCLEncoder unused_enc_;
  MCLExpEncoder quant_enc_;
};

extern GridIOOverlay grid_io_overlay;

#endif // PLATFORM_TBD
