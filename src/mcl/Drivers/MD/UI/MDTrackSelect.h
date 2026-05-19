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
  bool state;

  MDTrackSelect() : MidiSysexListenerClass(NULL, 0x7F, 0x0E) {}
  void setup(MidiClass *_midi);

  bool is_key_interface();
  bool on();
  bool off();

  virtual void start();
  virtual void end();
  void cleanup();

  // Message handlers
  void handle_track_select_legacy(const SysexView &view, uint8_t len);
  /* @} */
};

extern MDTrackSelect md_track_select;

#endif /* MDTRACKSELECT_H__ */
