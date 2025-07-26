#ifndef MNM_SYSEX_H__
#define MNM_SYSEX_H__

#include "Circular.h"
#include "Elektron.h"
#include "Midi.h"
#include "MidiSysex.h"
#include "Vector.h"
#include "WProgram.h"

class MNMSysexListenerClass : public ElektronSysexListenerClass {
public:
  bool isMNMMessage;

  MNMSysexListenerClass() : ElektronSysexListenerClass() {
    ids[0] = 0;
    ids[1] = 0x20;
    ids[2] = 0x3c;
  }

  virtual void start();
  virtual void handleByte(uint8_t byte);
  virtual void end();

  void setup(MidiClass *_midi);
};

extern MNMSysexListenerClass MNMSysexListener;

#endif /* MNM_SYSEX_H__ */
