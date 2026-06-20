#include "Sequencer/PtcVoiceRouter.h"

#include "Sequencer/SeqPtcTrackRef.h"
#if defined(__AVR__)
#include "../Drivers/MD/MD.h"
#endif

namespace {

bool ptc_voice_track_allocatable(uint8_t track) {
#if defined(__AVR__)
  return MD.getKitModelTuning(track) != nullptr;
#else
  return SeqPtcTrackRef::is_poly_voice_track(track);
#endif
}

static constexpr uint8_t PTC_ROUTE_NEUTRAL_FINE_TUNE = 32;

} // namespace

void PtcVoiceRouter::reset() {
  for (uint8_t x = 0; x < PTC_GROUP_TRACKS; x++) {
    voice_pitch[x] = 255;
    voice_order[x] = 0;
    voice_active[x] = false;
  }
}

uint8_t PtcVoiceRouter::get_next_voice(uint8_t pitch, uint8_t track_number,
                                       bool allow_direct) {
  if (track_number >= PTC_GROUP_TRACKS) {
    return 255;
  }

  uint16_t voice_mask = ptc_groups.mask_for_track(track_number);
  if (!voice_mask) {
    if (!allow_direct) {
      return 255;
    }
    voice_active[track_number] = true;
    voice_pitch[track_number] = pitch;
    return track_number;
  }

  uint16_t candidate_mask = 0;
  uint8_t active_same_pitch = 255;
  uint8_t inactive_same_pitch = 255;
  uint8_t inactive_voice = 255;
  uint8_t oldest_voice = 255;
  uint8_t oldest_val = 0;

  uint16_t voice_bit = 1;
  for (uint8_t x = 0; voice_mask; x++, voice_mask >>= 1, voice_bit <<= 1) {
    if (!(voice_mask & 1) || !ptc_voice_track_allocatable(x)) {
      continue;
    }
    candidate_mask |= voice_bit;
    if (voice_active[x]) {
      if (voice_pitch[x] == pitch) {
        active_same_pitch = x;
      }
      if (oldest_voice == 255 || voice_order[x] > oldest_val) {
        oldest_voice = x;
        oldest_val = voice_order[x];
      }
      continue;
    }
    if (voice_pitch[x] == pitch) {
      inactive_same_pitch = x;
    }
    inactive_voice = x;
  }

  uint8_t voice = 255;
  if (active_same_pitch != 255) {
    voice = active_same_pitch;
  } else if (inactive_same_pitch != 255) {
    voice = inactive_same_pitch;
  } else if (inactive_voice != 255) {
    voice = inactive_voice;
  } else {
    voice = oldest_voice;
  }

  if (voice == 255) {
    return 255;
  }

  uint8_t selected_order = voice_order[voice];
  for (uint8_t x = 0; candidate_mask; x++, candidate_mask >>= 1) {
    if ((candidate_mask & 1) && voice_order[x] <= selected_order &&
        x != voice) {
      voice_order[x]++;
    }
  }

  voice_order[voice] = 0;
  voice_pitch[voice] = pitch;
  voice_active[voice] = true;

  return voice;
}

uint8_t PtcVoiceRouter::release_voice(uint8_t pitch, uint8_t track_number,
                                      bool allow_direct) {
  if (track_number >= PTC_GROUP_TRACKS) {
    return 255;
  }

  uint16_t voice_mask = ptc_groups.mask_for_track(track_number);
  if (!voice_mask) {
    if (!allow_direct) {
      return 255;
    }
    voice_active[track_number] = false;
    return track_number;
  }

  for (uint8_t x = 0; voice_mask; x++, voice_mask >>= 1) {
    if ((voice_mask & 1) && ptc_voice_track_allocatable(x) &&
        voice_active[x] && voice_pitch[x] == pitch) {
      voice_active[x] = false;
      return x;
    }
  }

  return 255;
}

void PtcVoiceRouter::note_on(uint8_t route_channel, uint8_t note,
                             MidiUartClass *uart_) {
  (void)uart_;
  uint8_t track = ptc_route_channel_track(route_channel);
  if (SeqPtcTrackRef::is_midi_voice_track(track)) {
    SeqPtcTrackRef::trigger_voice(track, note, PTC_ROUTE_NEUTRAL_FINE_TUNE);
    return;
  }

  uint8_t voice = get_next_voice(note, track, true);
  if (voice >= PTC_GROUP_TRACKS) {
    return;
  }
  SeqPtcTrackRef::trigger_voice(voice, note, PTC_ROUTE_NEUTRAL_FINE_TUNE);
}

void PtcVoiceRouter::note_off(uint8_t route_channel, uint8_t note) {
  uint8_t track = ptc_route_channel_track(route_channel);
  if (SeqPtcTrackRef::is_midi_voice_track(track)) {
    SeqPtcTrackRef::release_voice(track);
    return;
  }

  uint8_t voice = release_voice(note, track, true);
  if (voice >= PTC_GROUP_TRACKS) {
    return;
  }
  SeqPtcTrackRef::release_voice(voice);
}

void PtcVoiceRouter::control_change(uint8_t route_channel, uint8_t cc,
                                    uint8_t value, MidiUartClass *uart_) {
  (void)uart_;
  if (cc < 16 || cc > 39) {
    return;
  }

  uint8_t track = ptc_route_channel_track(route_channel);
  uint8_t param = cc - 16;
  uint16_t mask = ptc_groups.mask_for_track(track);
  if (!mask) {
    SeqPtcTrackRef::set_route_param(track, param, value);
    return;
  }

  for (uint8_t n = 0; mask; n++, mask >>= 1) {
    if (mask & 1) {
      SeqPtcTrackRef::set_route_param(n, param, value);
    }
  }
}

PtcVoiceRouter ptc_voice_router;
