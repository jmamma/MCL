#ifndef MNM_SYSEX_H__
#define MNM_SYSEX_H__

#include "Circular.h"
#include "Elektron.h"
#include "Midi.h"
#include "MidiSysex.h"
#include "Vector.h"
#include "platform.h"

class MNMSysexListenerClass : public ElektronSysexListenerClass {
public:
  bool isMNMMessage;

  MNMSysexListenerClass() : ElektronSysexListenerClass() {}

  virtual void start();
  virtual void end();

  void setup(MidiClass *_midi);
};

extern MNMSysexListenerClass MNMSysexListener;

#endif /* MNM_SYSEX_H__ */
