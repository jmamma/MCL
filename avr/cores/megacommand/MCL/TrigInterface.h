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
  }

  bool on();
  bool off();

  virtual void start();
  virtual void end();
  virtual void end_immediate();
  void send_md_leds();
  void cleanup();
  /* @} */
};

extern TrigInterface trig_interface;

#endif /* TRIGINTERFACE_H__ */
