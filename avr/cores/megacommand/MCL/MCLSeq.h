/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MCLSEQUENCER_H__
#define MCLSEQUENCER_H__
#include "MCL.h"
#include "SeqPages.h"
#include "midiclock.h"

#define NUM_PARAM_PAGES 2
#define NUM_MD_TRACKS 16
#define NUM_EXT_TRACKS 4

class MCLSeq : public ClockCallback {
public:

  static uint8_t num_md_tracks = NUM_MD_TRACKS;
  static uint8_t num_ext_tracks = NUM_EXT_TRACKS;

  MDSeqTrack  md_tracks[NUM_MD_TRACKS];
  ExtSeqTrackExt ext_tracks[NUM_EXT_TRACKS];

  MCLSeqMidiEvents midi_events;

  void setup();
  void onMidiStopCallback();
  void sequencer();
  void trig_conditional();
  void send_parameter_locks(uint8_t i, uint8_t step_count);
  void seq_buffer_notesoff(uint8_t track);
  void seq_note_on(uint8_t track, uint8_t note);
  void seq_note_off(uint8_t track, uint8_t note);
  void noteon_conditional(uint8_t condition, uint8_t track, uint8_t note);

  void set_track_param(uint8_t track, uint8_t param, uint8_t value);
  void set_track_pitch(uint8_t track, uint8_t pitch);
  void set_track_step(uint8_t track, uint8_t step, uint8_t utiming,
                      uint8_t note_num, uint8_t velocity);

  void
  record_track(uint8_t track, uint8_t note_num,
               uint8_t velocity) void record_track_locks(uint8_t track,
                                                         uint8_t track_param,
                                                         uint8_t value);

  void record_ext_track_noteon(uint8_t track, uint8_t note_num,
                               uint8_t velocity);
  void record_ext_track_noteoff(uint8_t track, uint8_t note_num,
                                uint8_t velocity);

  void set_ext_track_step(uint8_t track, uint8_t step, uint8_t note_num,
                          uint8_t velocity);
};

extern MCLSeq mcl_seq;

class MCLSeqMidiEvents : public MidiCallBack {
  bool state;

  void setup_callbacks();
  void remove_callbacks();

  uint8_t note_to_trig(uint8_t);
  void onNoteOnCallback_Midi(uint8_t *msg);
  void onNoteOffCallback_Midi(uint8_t *msg);
  void onControlChangeCallback_Midi(uint8_t *msg);
  void onControlChangeCallback_Midi2(uint8_t *msg);
}

#endif /* MCLSEQUENCER_H__ */
