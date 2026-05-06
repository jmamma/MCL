#pragma once

#include "platform.h"

#ifdef PLATFORM_TBD

#include "../MidiDevice.h"

class TbdDevice : public MidiDevice {
public:
  TbdDevice();

  virtual bool probe() override;
  virtual void on_connection(uint8_t device_idx) override;
  virtual void init_grid_devices(uint8_t device_idx) override;
  virtual void ui_loop() override;
  virtual bool handle_ui_event(gui_event_t *event) override;
  virtual bool enter_ui(gui_event_t *event) override;
  virtual bool is_ui_active() override;
  virtual void exit_ui() override;

private:
  bool diag_active_ = false;
  uint8_t active_note_ = 255;

  void note_on(uint8_t note);
  void note_off();
};

extern TbdDevice TBD;

#endif // PLATFORM_TBD
