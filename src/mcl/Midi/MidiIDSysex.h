
#ifndef MIDIIDSYSEX_H__
#define MIDIIDSYSEX_H__

#include "MidiSysex.h"
class MidiClass;

class MidiIDSysexListenerClass : public MidiSysexListenerClass {
  /**
   * \addtogroup MidiID_sysex_listener
   *
   * @{
   **/

public:
  bool isIDMessage;
  MidiIDSysexListenerClass() : MidiSysexListenerClass() {
    ids[0] = 0x7E;
    ids[1] = 0x06;
    ids[2] = 0x02;
  }

  virtual void start();
  virtual void handleByte(uint8_t byte);
  virtual void end();
  void setup(MidiClass *_midi);
  void cleanup();
  /* @} */
};

extern MidiIDSysexListenerClass MidiIDSysexListener;

/* @} @} */

#endif /* MIDIIDSYSEX_H__ */
