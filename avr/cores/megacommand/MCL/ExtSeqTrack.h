/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef EXTSEQTRACK_H__
#define EXTSEQTRACK_H__

#define SEQ_NOTEBUF_SIZE 8

class ExtSeqTrack {

public:
  uint8_t channel;
  uint8_t port = UART2_PORT;
  MidiuUart* uart = &MidiUart2;
  uint8_t length;
  uint8_t mute;
  uint8_t resolution; // Resolution = 2 / ExtPatternResolution

  int8_t notes[4][128]; // 128 steps, up to 4 notes per step
  uint8_t
      notebuffer[SEQ_NOTEBUF_SIZE]; // we need to keep track of what notes are
                                    // currently being played, in order to stop
                                    // them in the event the sequencer stops
  void seq();

  uint8_t locks[4][128];
  uint8_t locks_params[4];
  uint64_t lock_masks[4];

  uint8_t conditional[128];
  uint8_t timing[128];

  void set_step(uint8_t step, uint8_t note_num,
 68                                 uint8_t velocity)
  void buffer_notesoff();
  void note_on(uint8_t note);
  void note_off(uint8_t note);
  void noteon_conditional(uint8_t condition, uint8_t note);
};

#endif /* EXTSEQTRACK_H__ */
