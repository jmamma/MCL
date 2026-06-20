/* Justin Mammarella jmamma@gmail.com 2021 */

#pragma once

#include "MidiUart.h"
#include "Sequencer/SeqTrack.h"
#include "platform.h"
#include "GridTrack.h"

//Ephemeral
class PerfSeqTrack : public SeqTrack  {

public:

  uint8_t perf_locks[4];

  PerfSeqTrack() : SeqTrack() {
    active = PERF_TRACK_TYPE;
 //   init();
  }
  void reset() {
    memset(perf_locks,255,sizeof(perf_locks));
    SeqTrack::reset();
  }

  void seq(MidiUartClass *uart_, MidiUartClass *uart2_);

};
