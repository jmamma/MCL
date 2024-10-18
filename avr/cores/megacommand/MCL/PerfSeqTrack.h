/* Justin Mammarella jmamma@gmail.com 2021 */

#pragma once

#include "MidiUartParent.h"
#include "SeqTrack.h"
#include "WProgram.h"
#include "GridTrack.h"

//Ephemeral
class PerfSeqTrack : public SeqTrackBase  {

public:

  uint8_t perf_locks[4];

  PerfSeqTrack() : SeqTrackBase() {
    active = PERF_TRACK_TYPE;
    init();
  }
  void reset() {
    memset(perf_locks,255,sizeof(perf_locks));
    SeqTrackBase::reset();
  }

  void seq(MidiUartParent *uart_, MidiUartParent *uart2_);

};
