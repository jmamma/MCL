#ifndef MDTRACKSELECT_H__
#define MDTRACKSELECT_H__

#include "NoteInterface.h"
#include "platform.h"
#include "MidiSysex.h"
class MidiClass;

// Track Select Protocol (F0 7F 0E ... F7)
// This handles legacy track select messages only.
// Machine updates and kit loaded now use Elektron sysex format (0x63, 0x54)

class MDTrackSelect : public MidiSysexListenerClass {

public:
  bool state = false;

  MDTrackSelect() : MidiSysexListenerClass() {
    ids[0] = 0x7F;
    ids[1] = 0x0E;
  }
  void setup(MidiClass *_midi);

  bool is_key_interface();
  bool on();
  bool off();

  virtual void start();
  virtual void end();
  void cleanup();

  // Message handlers
  void handle_track_select_legacy(uint8_t len);
  /* @} */
};

extern MDTrackSelect md_track_select;

#endif /* MDTRACKSELECT_H__ */
