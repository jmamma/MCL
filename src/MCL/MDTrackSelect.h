#ifndef MDTRACKSELECT_H__
#define MDTRACKSELECT_H__

#include "NoteInterface.h"
#include "WProgram.h"
#include "MidiSysex.h"
class MidiClass;

class MDTrackSelect : public MidiSysexListenerClass {

public:
  bool state = false;

  MDTrackSelect() : MidiSysexListenerClass() {
    ids[0] = 0x7F;
    ids[1] = 0x0E;
  }
  void setup(MidiClass *_midi);

  bool is_trig_interface();
  bool on();
  bool off();

  virtual void start();
  virtual void end();
  void cleanup();
  /* @} */
};

extern MDTrackSelect md_track_select;

#endif /* MDTRACKSELECT_H__ */
