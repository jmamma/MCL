/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MCLSEQUENCER_H__
#define MCLSEQUENCER_H__
#include "MCL.h"
#include "SeqPages.h"
#include "midiclock.h"
#define NUM_PARAM_PAGES 2;

class MCLSeq : public ClockCallback {
public:

  uint8_t PatternLengths[16];
  uint8_t PatternLocks[16][4][64];
  uint8_t PatternLocksParams[16][4];
  uint64_t PatternMasks[16];
  uint64_t LockMasks[16];
  uint8_t conditional[16][64];
  uint8_t timing[16][64];

  uint8_t ExtPatternMutes[6];
  uint8_t ExtPatternLengths[6];
  uint8_t ExtPatternResolution[6]; // Resolution = 2 / ExtPatternResolution

  int8_t ExtPatternNotes[6][4][128];
  uint8_t ExtPatternNoteBuffer[6][SEQ_NOTEBUF_SIZE];

  uint8_t ExtPatternLocks[4][4][128];
  uint8_t ExtPatterLockParams[4][4];
  uint64_t ExtLockMasks[4];

  uint8_t Extconditional[6][128];
  uint8_t Exttiming[6][128];

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
