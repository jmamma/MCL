#pragma once

#include "platform.h"

#ifdef PLATFORM_TBD

#include <stddef.h>
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
  bool get_track_default_presets(char *response, size_t response_len,
                                 const char *template_name = nullptr,
                                 uint32_t timeout_ms = 30000);
  bool get_macro_sound_preset(const char *preset_id, char *response,
                              size_t response_len,
                              uint32_t timeout_ms = 30000);
  bool get_macro_definition(const char *macro_id, char *response,
                            size_t response_len,
                            uint32_t timeout_ms = 30000);
  bool get_kit_index_json(char *response, size_t response_len,
                          uint32_t timeout_ms = 30000);
  bool set_active_sample_kit(uint8_t kit_index,
                             uint32_t timeout_ms = 30000);
  bool activate_track_machine(uint8_t track_index, const char *machine_id,
                              uint32_t timeout_ms = 30000);
  bool load_track_sound_preset(uint8_t track_index, const char *preset_id,
                               uint8_t rom_bank = 0xFF,
                               int32_t sample_slice = -1,
                               uint32_t timeout_ms = 30000);
  bool load_track_macro_definition(uint8_t track_index, const char *macro_id,
                                   uint32_t timeout_ms = 30000);
  bool set_track_mute(uint8_t track_index, bool muted,
                      uint32_t timeout_ms = 100);

  void get_stats(TbdP4CommandStats &stats) const;

private:
  void reset_frame(uint8_t request);
  bool transfer_frame(uint32_t timeout_ms);
  bool send_frame(uint8_t request, uint32_t timeout_ms);
  bool receive_response(uint8_t request, char *response, size_t response_len,
                        uint32_t timeout_ms);

  bool initialized_ = false;
  uint32_t tx_frames_ = 0;
  uint32_t ready_timeouts_ = 0;
  uint8_t last_request_ = 0;
};

extern TbdP4CommandTransport tbd_p4_command;

#endif // PLATFORM_TBD
