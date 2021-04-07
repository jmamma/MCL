#ifndef TRIGINTERFACE_H__
#define TRIGINTERFACE_H__

#include "NoteInterface.h"
#include "WProgram.h"

#define MDX_KEY_TRIG1 0x00
#define MDX_KEY_ENCBUTTON1 0x10
#define MDX_KEY_BANKA 0x1A
#define MDX_KEY_BANKB 0x1B
#define MDX_KEY_BANKC 0x1C
#define MDX_KEY_BANKD 0x1D
#define MDX_KEY_BANKGROUP 0x1E
#define MDX_KEY_SONG 0x1F
#define MDX_KEY_REC 0x20
#define MDX_KEY_FUNC 0x25
#define MDX_KEY_LEFT 0x26
#define MDX_KEY_RIGHT 0x27
#define MDX_KEY_YES 0x28
#define MDX_KEY_NO 0x29
#define MDX_KEY_SCALE 0x2A
#define MDX_KEY_UP 0x30
#define MDX_KEY_DOWN 0x31
#define MDX_KEY_COPY 0x34
#define MDX_KEY_CLEAR 0x35
#define MDX_KEY_PASTE 0x36
#define MDX_KEY_REALTIME 0x37

class TrigInterface : public MidiSysexListenerClass {

public:
  bool state = false;
  uint64_t cmd_key_state;
  uint64_t ignore_next_mask;

  TrigInterface() : MidiSysexListenerClass() {
    ids[0] = 0x7F;
    ids[1] = 0x0D;
  }
  void setup(MidiClass *_midi) {
    sysex = &(_midi->midiSysex);
  }
  void ignoreNextEvent(uint8_t i) {
    SET_BIT64(ignore_next_mask, i);
  }
  bool on();
  bool off();

  void enable_listener();
  void disable_listener();

  virtual void start();
  virtual void end();
  virtual void end_immediate();
  bool is_key_down(uint8_t key) { return IS_BIT_SET64(cmd_key_state, key); }
  void send_md_leds(TrigLEDMode mode = TRIGLED_EXCLUSIVE);
  void cleanup();
  /* @} */
};

extern TrigInterface trig_interface;

#endif /* TRIGINTERFACE_H__ */
