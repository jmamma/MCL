#ifndef TRIGINTERFACE_H__
#define TRIGINTERFACE_H__

#include "NoteInterface.h"
#include "WProgram.h"
#include "MidiSysex.h"
#include "Elektron.h"

#define MDX_KEY_TRIG1 0x00
#define MDX_KEY_ENCBUTTON1 0x10
#define MDX_KEY_BANKA 0x1A
#define MDX_KEY_BANKB 0x1B
#define MDX_KEY_BANKC 0x1C
#define MDX_KEY_BANKD 0x1D
#define MDX_KEY_BANKGROUP 0x1E
#define MDX_KEY_PATSONG 0x1F
#define MDX_KEY_REC 0x20
#define MDX_KEY_GLOBAL 0x21
#define MDX_KEY_PATSONGKIT 0x22
#define MDX_KEY_KIT 0x23
#define MDX_KEY_FUNC 0x25
#define MDX_KEY_LEFT 0x26
#define MDX_KEY_RIGHT 0x27
#define MDX_KEY_YES 0x28
#define MDX_KEY_NO 0x29
#define MDX_KEY_SCALE 0x2A
#define MDX_KEY_MUTE 0x2B
#define MDX_KEY_SONG 0x2C
#define MDX_KEY_EXTENDED 0x2D
#define MDX_KEY_UP 0x30
#define MDX_KEY_DOWN 0x31
#define MDX_KEY_STOP 0x32
#define MDX_KEY_COPY 0x34
#define MDX_KEY_CLEAR 0x35
#define MDX_KEY_PASTE 0x36
#define MDX_KEY_REALTIME 0x37
#define MDX_KEY_FUNCYES 0x3A
#define MDX_KEY_FUNCEXTENDED 0x3B

#include "Task.h"

/*
#define KEY_REPEAT_INTERVAL 80

class TrigInterfaceTask : public Task {

public:

  TrigInterfaceTask() : Task(KEY_REPEAT_INTERVAL) { 
  }

  void setup() {
    interval = KEY_REPEAT_INTERVAL; 
    starting = false;
    uint16_t clock = read_slowclock();
    lastExecution = clock;
  } 

  virtual void run();

};

extern TrigInterfaceTask trig_interface_task;
*/
class MidiClass;

class TrigInterface : public MidiSysexListenerClass {

public:
  bool state = false;
  uint64_t cmd_key_state;
  uint64_t ignore_next_mask;
  uint16_t last_clock;
  bool throttle;

  TrigInterface() : MidiSysexListenerClass() {
    ids[0] = 0x7F;
    ids[1] = 0x0D;
  }
  void setup(MidiClass *_midi);
  void ignoreNextEvent(uint8_t i) {
    SET_BIT64(ignore_next_mask, i);
  }
  void ignoreNextEventClear(uint8_t i) {
    CLEAR_BIT64(ignore_next_mask, i);
  }
  bool on(bool clear_states = true);
  bool off();

  bool check_key_throttle();
  void enable_listener();
  void disable_listener();
  virtual void start();
  virtual void end();
  bool is_key_down(uint8_t key) { return IS_BIT_SET64(cmd_key_state, key); }
  void send_md_leds(TrigLEDMode mode = TRIGLED_EXCLUSIVE);
  void cleanup();
  /* @} */
};

extern TrigInterface trig_interface;

#endif /* TRIGINTERFACE_H__ */
