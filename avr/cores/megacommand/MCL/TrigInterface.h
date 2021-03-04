#ifndef TRIGINTERFACE_H__
#define TRIGINTERFACE_H__

#include "NoteInterface.h"
#include "WProgram.h"

#define MDX_KEY_FUNC 0x24 + 1
#define MDX_KEY_LEFT 0x25 + 1
#define MDX_KEY_RIGHT 0x26 + 1
#define MDX_KEY_YES 0x27 + 1
#define MDX_KEY_NO 0x28 + 1
#define MDX_KEY_SCALE 0x29 + 1
#define MDX_KEY_UP 0x2F + 1
#define MDX_KEY_DOWN 0x30 + 1

class TrigInterface : public MidiSysexListenerClass {

public:
  bool state = false;
  uint64_t cmd_key_state;

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
  void send_md_leds(TrigLEDMode mode = TRIGLED_EXCLUSIVE);
  void cleanup();
  /* @} */
};

extern TrigInterface trig_interface;

#endif /* TRIGINTERFACE_H__ */
