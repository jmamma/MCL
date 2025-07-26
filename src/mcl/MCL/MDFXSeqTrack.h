/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MDFXSEQTRACK_H__
#define MDFXSEQTRACK_H__

#include "MCLMemory.h"
// #include "MidiActivePeering.h"
#include "MidiUartParent.h"
#include "WProgram.h"

class MDFXSeqTrack : public SeqTrackBase {

public:
  MDFXSeqTrack() { SeqTrackBase::reset(); }

  ALWAYS_INLINE() void seq() {
    uint8_t timing_mid = get_timing_mid();
    mod12_counter++;
    if (mod12_counter == timing_mid) {
      mod12_counter = 0;
      step_count_inc();
    }
    if (count_down) {
      count_down--;
      if (count_down == 0) {
        if (load_sound) {
          MDSeqTrack::load_machine_cache |= ((uint32_t) 0b1111 << 16);
          load_sound = 0;
        }
        reset();
        mod12_counter = 0;
      }
    }

  }
};
#endif /* MDFXSEQTRACK_H__ */
