/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MCLSEQUENCER_H__
#define MCLSEQUENCER_H__
#include "MCL.h"
#include "midiclock.h"
class MCLSequencer : public ClockCallback {
public:
  void setup();
  void onMidiStopCallback();
  void sequencer();
  void trig_conditional();
  void parameter_locks(uint8_t i, uint8_t step_count);
  void seq_buffer_notesoff(uint8_t track);
  void seq_note_on(uint8_t track, uint8_t note);
  void seq_note_off(uint8_t track, uint8_t note);
  void noteon_conditional(uint8_t condition, uint8_t track, uint8_t note);
};

extern MCLSequencer mcl_seq;
#endif /* MCLSEQUENCER_H__ */
