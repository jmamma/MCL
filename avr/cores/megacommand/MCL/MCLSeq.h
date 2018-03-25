/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MCLSEQUENCER_H__
#define MCLSEQUENCER_H__

#include "SeqPages.h"
#include "ExtSeqTrack.h"
#include "MDSeqTrack.h"
#include "midi-common.hh"

//#include "MDTrack.h"
#define NUM_MD_TRACKS 16
#define NUM_EXT_TRACKS 4

#define SEQ_MUTE_ON 1
#define SEQ_MUTE_OFF 0

class MCLSeqMidiEvents : public MidiCallback {
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
  uint8_t num_ext_tracks = NUM_EXT_TRACKS;

  MDSeqTrack  md_tracks[NUM_MD_TRACKS];
  ExtSeqTrack ext_tracks[NUM_EXT_TRACKS];

  MCLSeqMidiEvents midi_events;

  void setup();
  void onMidiStopCallback();
  void sequencer();
};

extern MCLSeq mcl_seq;


#endif /* MCLSEQUENCER_H__ */
