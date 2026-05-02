/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#pragma once

#include "MCLMemory.h"
#include "MidiClock.h"
#include "Midi.h"

#include "MDSeqTrack.h"
#include "LFOSeqTrack.h"
#include "ArpSeqTrack.h"
#include "ExtSeqTrack.h"
#include "MDFXseqTrack.h"
#include "PerfSeqTrack.h"
#if !defined(__AVR__)
#include "SPSXSeqTrack.h"
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

  MidiUartClass *md_uart = &MidiUart;
  MidiUartClass *ext_uart = &MidiUart2;

  static constexpr uint8_t num_md_tracks = NUM_MD_TRACKS;
  MDSeqTrack md_tracks[NUM_MD_TRACKS];
  MDArpSeqTrack md_arp_tracks[NUM_MD_TRACKS];

#if !defined(__AVR__)
  SPSXSeqTrack spsx_tracks[NUM_MD_TRACKS];
  bool using_spsx_tracks = false;
  uint16_t neighbor_trig_mask = 0;
  uint16_t fill_mask = 0;
  void switch_to_spsx();
  void switch_to_legacy();
#endif

#ifdef EXT_TRACKS
  ExtSeqTrack ext_tracks[NUM_EXT_TRACKS];
  ExtArpSeqTrack ext_arp_tracks[NUM_EXT_TRACKS];

  static constexpr uint8_t num_ext_tracks = NUM_EXT_TRACKS;
#endif

#ifdef LFO_TRACKS
  LFOSeqTrack lfo_tracks[NUM_LFO_TRACKS];
  static constexpr uint8_t num_lfo_tracks = NUM_LFO_TRACKS;
#endif

  SeqTrack aux_tracks[NUM_AUX_TRACKS];

  PerfSeqTrack perf_track;
  MDFXSeqTrack mdfx_track;

  MCLSeqMidiEvents midi_events;

  bool state = false;

  void enable() { state = true; }
  void disable() { state = false; }

  void setup();
  void set_ports(MidiUartClass *md_uart_, MidiUartClass *ext_uart_);

  uint8_t find_ext_track(uint8_t channel);

  void update_kit_params();
  void update_params();
  void onMidiStartCallback();
  void onMidiStartImmediateCallback();
  void onMidiContinueCallback();
  void onMidiStopCallback();
  void seq();
};

extern MCLSeq mcl_seq;
