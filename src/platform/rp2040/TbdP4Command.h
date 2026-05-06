#pragma once

#include "platform.h"

#ifdef PLATFORM_TBD

#include <stdint.h>

struct TbdP4CommandStats {
  uint32_t tx_frames;
  uint32_t ready_timeouts;
  uint8_t last_request;
  bool initialized;
  bool ready_pin;
};

class TbdP4CommandTransport {
public:
  void init();

  bool ready() const;
  bool wait_ready(uint32_t timeout_ms = 30000) const;

  bool announce_app(const char *app_name, uint8_t flags = 0,
                    uint32_t timeout_ms = 30000);
  bool load_track_sound_preset(uint8_t track_index, const char *preset_id,
                               uint8_t rom_bank = 0xFF,
                               int32_t sample_slice = -1,
                               uint32_t timeout_ms = 30000);

  void get_stats(TbdP4CommandStats &stats) const;

private:
  void reset_frame(uint8_t request);
  bool send_frame(uint8_t request, uint32_t timeout_ms);

  bool initialized_ = false;
  uint32_t tx_frames_ = 0;
  uint32_t ready_timeouts_ = 0;
  uint8_t last_request_ = 0;
};

extern TbdP4CommandTransport tbd_p4_command;

#endif // PLATFORM_TBD
