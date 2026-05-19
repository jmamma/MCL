#pragma once

#include <inttypes.h>

class MidiClass;
class MidiUartClass;

class SeqPtcTrackRef {
public:
  static uint8_t track_count();
  static bool is_poly_voice_track(uint8_t track);
  static bool is_midi_voice_track(uint8_t track);
  static bool can_polylink_param(uint8_t source_track, uint8_t target_track,
                                 uint8_t param);
  static bool is_mute_param(uint8_t param);

  static uint8_t note_from_pitch(uint8_t track, uint8_t pitch);
  static uint8_t pitch_from_note(uint8_t track, uint8_t note,
                                 uint8_t fine_tune = 255);

  static bool parse_cc(uint8_t channel, uint8_t cc, uint8_t *track,
                       uint8_t *param);
  static bool set_param(uint8_t track, uint8_t param, uint8_t value,
                        MidiUartClass *uart_ = nullptr,
                        bool update_kit = false);
  static bool set_pitch(uint8_t track, uint8_t pitch,
                        MidiUartClass *uart_ = nullptr);
  static void trigger(uint8_t track, uint8_t velocity,
                      MidiUartClass *uart_ = nullptr);
  static bool trigger_voice(uint8_t track, uint8_t note,
                            uint8_t fine_tune = 255,
                            MidiUartClass *uart_ = nullptr,
                            uint8_t *record_pitch = nullptr);
  static bool release_voice(uint8_t track);

  static void send_notes_on(uint8_t track);
  static void send_notes_off(uint8_t track);
  static void send_notes(uint8_t track, uint8_t pitch);
  static void record_track(uint8_t track, uint8_t velocity);
  static void record_pitch(uint8_t track, uint8_t pitch);

  static bool copy_track_label(uint8_t track, char *out, uint8_t len);
  static void popup_text(char *text);
  static MidiClass *param_midi();
};
