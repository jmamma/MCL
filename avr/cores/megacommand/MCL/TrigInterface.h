#ifndef TRIGINTERFACE_H__
#define TRIGINTERFACE_H__

#include "NoteInterface.h"
#include "WProgram.h"

class TrigInterface : public MidiSysexListenerClass {

public:
  bool state = false;

  TrigInterface() : MidiSysexListenerClass() {
    ids[0] = 0x7F;
    ids[1] = 0x0D;
  }
  void setup(MidiClass *_midi) {
    sysex = &(_midi->midiSysex);
    sysex->addSysexListener(this);
  }

  bool is_trig_interface();
  bool on();
  bool off();

  void activate_trig_interface();
  void deactivate_trig_interface();

  virtual void start();
  virtual void end();
  virtual void end_immediate();
  void cleanup();
  /* @} */
};

extern TrigInterface trig_interface;

#endif /* TRIGINTERFACE_H__ */
