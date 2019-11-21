/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MCLSEQUENCER_H__
#define MCLSEQUENCER_H__

#include "ExtSeqTrack.h"
#include "LFOSeqTrack.h"
#include "MCLMemory.h"
#include "MDSeqTrack.h"
#include "SeqPages.h"
#include "midi-common.hh"

//#include "MDTrack.h"
#define SEQ_MUTE_ON 1
#define SEQ_MUTE_OFF 0

class MCLSeqMidiEvents : public MidiCallback {
public:
  bool state;

  void setup_callbacks();
  void remove_callbacks();

  uint8_t note_to_trig(uint8_t);
  void onNoteOnCallback_Midi(uint8_t *msg);
  void onNoteOffCallback_Midi(uint8_t *msg);
  void onControlChangeCallback_Midi(uint8_t *msg);
  void onControlChangeCallback_Midi2(uint8_t *msg);
};

class MCLSeq : public ClockCallback {
public:
  uint8_t num_md_tracks = NUM_MD_TRACKS;
  MDSeqTrack md_tracks[NUM_MD_TRACKS];

#ifdef EXT_TRACKS
  ExtSeqTrack ext_tracks[NUM_EXT_TRACKS];
  uint8_t num_ext_tracks = NUM_EXT_TRACKS;
#endif

  LFOSeqTrack lfo_tracks[NUM_LFO_TRACKS];
  uint8_t num_lfo_tracks = NUM_LFO_TRACKS;

  MCLSeqMidiEvents midi_events;
  bool state = false;

  void setup();
  void enable();
  void disable();

  void update_kit_params();
  void update_params();
  void onMidiStartCallback();
  void onMidiStartImmediateCallback();
  void onMidiContinueCallback();
  void onMidiStopCallback();
  void seq();
};

extern MCLSeq mcl_seq;

#endif /* MCLSEQUENCER_H__ */
