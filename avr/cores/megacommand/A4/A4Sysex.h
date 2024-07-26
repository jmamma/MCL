
#ifndef A4SYSEX_H__
#define A4SYSEX_H__

#include "A4.h"
#include "Callback.h"
#include "Midi.h"
#include "MidiSysex.h"
#include "Vector.h"
#include "WProgram.h"

class A4SysexListenerClass : public ElektronSysexListenerClass {
  /**
   * \addtogroup A4_sysex_listener
   *
   * @{
   **/

public:
  /** Stores if the currently received message is a MachineDrum sysex message.
   * **/
  bool isA4Message;

  A4SysexListenerClass() : ElektronSysexListenerClass() {
    ids[0] = 0;
    ids[1] = 0x20;
    ids[2] = 0x3c;
  }

  virtual void start();
  virtual void handleByte(uint8_t byte);
  virtual void end();
  /**
   * Add the sysex listener to the MIDI sysex subsystem. This needs to
   * be called if you want to use the A4SysexListener (it is called
   * automatically by the A4Task subsystem though).
   **/
  void setup(MidiClass *_midi);
  /* @} */
};

#include "A4Messages.h"

extern A4SysexListenerClass A4SysexListener;

/* @} @} */

#endif /* A4SYSEX_H__ */
