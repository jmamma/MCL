
#ifndef MIDIIDSYSEX_H__
#define MIDIIDSYSEX_H__

#include "Midi.h"
#include "MidiSysex.hh"
#include "WProgram.h"

class MidiIDSysexListenerClass : public MidiSysexListenerClass {
  /**
   * \addtogroup MidiID_sysex_listener
   *
   * @{
   **/

public:
  bool isIDMessage;
  uint8_t msgType;
  MidiIDSysexListenerClass() : MidiSysexListenerClass() {
    ids[0] = 0x7E;
    ids[1] = 0x06;
    ids[2] = 0x02;
  }

  virtual void start();
  virtual void handleByte(uint8_t byte);
  virtual void end();
  virtual void end_immediate();
  void setup();
  void cleanup();
  /* @} */
};

extern MidiIDSysexListenerClass MidiIDSysexListener;

/* @} @} */

#endif /* MIDIIDSYSEX_H__ */
