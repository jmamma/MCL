#pragma once

#include "MegaComTask.h"

enum ui_server_command_t {
  USC_DRAW, // 512B framebuffer, could use some compression here
  USC_INVALIDATE_VISUALS,
  USC_KEYBOARD, // text input
  USC_BUTTON, // button_id, event_type
  USC_ENCODER, // encoder_id, event_type
  USC_TRIG_INTERFACE,
};

class MCUIServer : public MegaComServer {
private:
  bool m_update;
public:
  MCUIServer(): MegaComServer(false), m_update(false) { }
  virtual int run();
  virtual int resume(int);
  void update(); // driven by the GUI loop
};

extern MCUIServer megacom_uiserver;
