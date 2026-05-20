/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#pragma once

#include "MCLMemory.h"
#include "MidiClock.h"
#include "Midi.h"

#include "MDSeqTrack.h"
#include "LFOSeqTrack.h"
#include "ArpSeqTrack.h"
#include "ExtSeqTrack.h"
#include "MDFXSeqTrack.h"
#include "PerfSeqTrack.h"
#if !defined(__AVR__)
#include "SPSXSeqTrack.h"
#endif
#if !defined(__AVR__)
#include "../Drivers/Generic/Sequencer/MidiSeqTrack.h"
#endif
#if defined(PLATFORM_TBD)
#include "../Drivers/TBD/TBDSeqTrack.h"
#endif

#define SEQ_MUTE_ON 1
#define SEQ_MUTE_OFF 0

#define SEQ_SCALE_1X 0
#define SEQ_SCALE_2X 1
#define SEQ_SCALE_3_4X 2
#define SEQ_SCALE_3_2X 3
#define SEQ_SCALE_1_2X 4
#define SEQ_SCALE_1_4X 5
#define SEQ_SCALE_1_8X 6

#define NUM_TRIG_CONDITIONS 14

class MCLSeqMidiEvents : public MidiCallback {
public:
  bool state;
  void setup_callbacks();
  void remove_callbacks();

  uint8_t note_to_trig(uint8_t);
  void onNoteCallback_Midi(uint8_t *msg);

  void onNoteOffCallback_Midi(uint8_t *msg);
  void onControlChangeCallback_Midi(uint8_t *msg);
  void onControlChangeCallback_Midi2(uint8_t *msg);
};

class MCLSeq : public ClockCallback {
public:
  bool uart_sidechannel;
  bool realtime;

  // Logical sequencer outputs; both may resolve to the same physical transport.
  MidiUartClass *primary_output = &MidiUart;
  MidiUartClass *secondary_output = &MidiUart2;

  static constexpr uint8_t num_md_tracks = NUM_MD_TRACKS;
  MDSeqTrack md_tracks[NUM_MD_TRACKS];
  MDArpSeqTrack md_arp_tracks[NUM_MD_TRACKS];

#if !defined(__AVR__)
  SPSXSeqTrack spsx_tracks[NUM_MD_TRACKS];
  // Engine-mode flag: true when the sequencer is running the SPSX track
  // engine (spsx_tracks[]) instead of legacy md_tracks[]. Distinct from
  // MD.is_spsx (firmware capability) — readers asking "what does the engine
  // store?" should test this, not MD.is_spsx.
  bool using_spsx_tracks = false;
  uint16_t neighbor_trig_mask = 0;
  uint16_t fill_mask = 0;
  // Mode switch — only safe when MidiClock is PAUSED. Returns false if
  // refused (transport playing). Callers must stop transport first.
  bool switch_to_spsx();
  bool switch_to_legacy();
  void set_fill(bool held) { fill_mask = held ? 0xFFFF : 0; }
  void set_fill_track(uint8_t track, bool held) {
    if (held) fill_mask |= (uint16_t)(1u << track);
    else fill_mask &= (uint16_t)~(1u << track);
  }
#else
  // AVR has no SPSX engine. Provide a constant so portable readers compile.
  static constexpr bool using_spsx_tracks = false;
#endif

#if defined(PLATFORM_TBD)
  static constexpr uint8_t num_tbd_tracks = TBD_P4_SOUND_TRACK_COUNT;
  TBDSeqTrack tbd_tracks[TBD_P4_SOUND_TRACK_COUNT];
#endif
#if !defined(__AVR__)
  static constexpr uint8_t num_midi_tracks = NUM_EXT_TRACKS;
  MidiSeqTrack midi_tracks[NUM_EXT_TRACKS];
#endif

#ifdef EXT_TRACKS
  ExtSeqTrack ext_tracks[NUM_EXT_TRACKS];
  ExtArpSeqTrack ext_arp_tracks[NUM_EXT_TRACKS];

  static constexpr uint8_t num_ext_tracks = NUM_EXT_TRACKS;
#endif

#ifdef LFO_TRACKS
  LFOSeqTrack grid_x_lfo_tracks[NUM_GRID_X_LFO_TRACKS];
  static constexpr uint8_t num_grid_x_lfo_tracks = NUM_GRID_X_LFO_TRACKS;
#ifdef EXT_TRACKS
  LFOSeqTrack grid_y_lfo_tracks[NUM_GRID_Y_LFO_TRACKS];
  static constexpr uint8_t num_grid_y_lfo_tracks = NUM_GRID_Y_LFO_TRACKS;
#endif
  uint16_t lfo_track_trig_mask_primary;
  uint16_t lfo_track_trig_mask_secondary;

  void set_lfo_track_trig(DeviceIdx device_idx, uint8_t track) {
    if (track >= 16) {
      return;
    }
    uint16_t bit = (uint16_t)1 << track;
    if (device_idx == DeviceIdx::Secondary) {
      lfo_track_trig_mask_secondary |= bit;
    } else {
      lfo_track_trig_mask_primary |= bit;
    }
  }

  bool lfo_track_trig_fired(DeviceIdx device_idx, uint8_t track) const {
    if (track >= 16) {
      return false;
    }
    uint16_t mask = device_idx == DeviceIdx::Secondary
                        ? lfo_track_trig_mask_secondary
                        : lfo_track_trig_mask_primary;
    return (mask & ((uint16_t)1 << track)) != 0;
  }

  void clear_lfo_track_trigs(DeviceIdx device_idx) {
    if (device_idx == DeviceIdx::Secondary) {
      lfo_track_trig_mask_secondary = 0;
    } else {
      lfo_track_trig_mask_primary = 0;
    }
  }
#endif

  SeqTrack aux_tracks[NUM_AUX_TRACKS];

  PerfSeqTrack perf_track;
  MDFXSeqTrack mdfx_track;

  MCLSeqMidiEvents midi_events;

  bool state;
#if !defined(__AVR__)
  uint8_t legacy_tick_counter = 0;
#endif

  void enable() { state = true; }
  void disable() { state = false; }

  void setup();
  void set_outputs(MidiUartClass *primary_output_,
                   MidiUartClass *secondary_output_);

  uint8_t find_ext_track(uint8_t channel);

  void update_kit_params();
  void update_params();
  void onMidiStartCallback();
  void onMidiStartImmediateCallback();
  void onMidiContinueCallback();
  void onMidiStopCallback();
  void configure_clock_interpolation();
  bool legacy_tick_due();
  void seq();
};

extern MCLSeq mcl_seq;
