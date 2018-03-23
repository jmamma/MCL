/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef EXTSEQTRACK_H__
#define EXTSEQTRACK_H__

#include "MidiUartParent.hh"
//#include "MidiUart.h"
#include "WProgram.h"
#define SEQ_NOTEBUF_SIZE 8
#define SEQ_MUTE_ON 1
#define SEQ_MUTE_OFF 0

#define UART2_PORT 2

class ExtSeqTrackData {
public:
  uint8_t length = 16;
  uint8_t resolution = 1; // Resolution = 2 / ExtPatternResolution

  int8_t notes[4][128]; // 128 steps, up to 4 notes per step
  uint8_t locks[4][128];
  uint8_t locks_params[4];
  uint64_t lock_masks[4];

  uint8_t conditional[128];
  uint8_t timing[128];
};
class ExtSeqTrack : public ExtSeqTrackData {

public:
  uint8_t channel;
  uint8_t port = UART2_PORT;
  MidiUartParent *uart = &MidiUart2;

  uint8_t mute = SEQ_MUTE_OFF;
  uint8_t
      notebuffer[SEQ_NOTEBUF_SIZE]; // we need to keep track of what notes are
                                    // currently being played, in order to stop
                                    // them in the event the sequencer stops
  void seq();
  void set_step(uint8_t step, uint8_t note_num, uint8_t velocity);
  void buffer_notesoff();
  void note_on(uint8_t note);
  void note_off(uint8_t note);
  void noteon_conditional(uint8_t condition, uint8_t note);
  void record_ext_track_noteon(uint8_t note_num, uint8_t velocity);
  void record_ext_track_noteoff(uint8_t note_num, uint8_t velocity);

  void set_ext_track_step(uint8_t step, uint8_t note_num, uint8_t velocity);

  void clear_ext_conditional();
  void clear_ext_notes();
  void clear_track();
};

#endif /* EXTSEQTRACK_H__ */
