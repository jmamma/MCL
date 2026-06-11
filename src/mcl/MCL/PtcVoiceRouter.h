#pragma once

#include "PtcGroups.h"
#include <stdint.h>

class MidiUartClass;

static constexpr uint8_t PTC_EXT_ROUTE_CHANNEL_BASE = 16;
static constexpr uint8_t PTC_EXT_ROUTE_CHANNEL_END =
    PTC_EXT_ROUTE_CHANNEL_BASE + PTC_GROUP_TRACKS;

static inline bool ptc_route_channel_is_primary(uint8_t channel) {
#if defined(__AVR__)
  return channel >= PTC_EXT_ROUTE_CHANNEL_BASE;
#else
  return channel >= PTC_EXT_ROUTE_CHANNEL_BASE &&
         channel < PTC_EXT_ROUTE_CHANNEL_END;
#endif
}

static inline uint8_t ptc_route_channel_track(uint8_t channel) {
  return channel - PTC_EXT_ROUTE_CHANNEL_BASE;
}

class PtcVoiceRouter {
public:
  void reset();

  uint8_t get_next_voice(uint8_t pitch, uint8_t track_number,
                         bool allow_direct);
  uint8_t release_voice(uint8_t pitch, uint8_t track_number,
                        bool allow_direct);

  void note_on(uint8_t route_channel, uint8_t note, MidiUartClass *uart_);
  void note_off(uint8_t route_channel, uint8_t note);
  void control_change(uint8_t route_channel, uint8_t cc, uint8_t value,
                      MidiUartClass *uart_);

private:
  uint8_t voice_pitch[PTC_GROUP_TRACKS];
  uint8_t voice_order[PTC_GROUP_TRACKS];
  bool voice_active[PTC_GROUP_TRACKS];
};

extern PtcVoiceRouter ptc_voice_router;
