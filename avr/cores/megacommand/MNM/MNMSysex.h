#ifndef MNM_SYSEX_H__
#define MNM_SYSEX_H__

#include "Circular.h"
#include "Elektron.h"
#include "Midi.h"
#include "MidiSysex.h"
#include "Vector.h"
#include "WProgram.h"

class MNMSysexListenerClass : public MidiSysexListenerClass {
public:
  CallbackVector<SysexCallback, 8> onGlobalMessageCallbacks;
  CallbackVector<SysexCallback, 8> onKitMessageCallbacks;
  CallbackVector<SysexCallback, 8> onSongMessageCallbacks;
  CallbackVector<SysexCallback, 8> onPatternMessageCallbacks;

  CallbackVector2<SysexCallback, 8, uint8_t, uint8_t> onStatusResponseCallbacks;

  bool isMNMMessage;
  uint8_t msgType;

  MNMSysexListenerClass() : MidiSysexListenerClass() {
    ids[0] = 0;
    ids[1] = 0x20;
    ids[2] = 0x3c;
  }

  virtual void start();
  virtual void handleByte(uint8_t byte);
  virtual void end_immediate();
  virtual void end();

  void setup(MidiClass *_midi);

  void addOnStatusResponseCallback(SysexCallback *obj,
                                   sysex_status_callback_ptr_t func) {
    onStatusResponseCallbacks.add(obj, func);
  }
  void removeOnStatusResponseCallback(SysexCallback *obj,
                                      sysex_status_callback_ptr_t func) {
    onStatusResponseCallbacks.remove(obj, func);
  }
  void removeOnStatusResponseCallback(SysexCallback *obj) {
    onStatusResponseCallbacks.remove(obj);
  }

  void addOnGlobalMessageCallback(SysexCallback *obj, sysex_callback_ptr_t func) {
    onGlobalMessageCallbacks.add(obj, func);
  }
  void removeOnGlobalMessageCallback(SysexCallback *obj,
                                     sysex_callback_ptr_t func) {
    onGlobalMessageCallbacks.remove(obj, func);
  }
  void removeOnGlobalMessageCallback(SysexCallback *obj) {
    onGlobalMessageCallbacks.remove(obj);
  }

  void addOnKitMessageCallback(SysexCallback *obj, sysex_callback_ptr_t func) {
    onKitMessageCallbacks.add(obj, func);
  }
  void removeOnKitMessageCallback(SysexCallback *obj, sysex_callback_ptr_t func) {
    onKitMessageCallbacks.remove(obj, func);
  }
  void removeOnKitMessageCallback(SysexCallback *obj) {
    onKitMessageCallbacks.remove(obj);
  }

  void addOnPatternMessageCallback(SysexCallback *obj, sysex_callback_ptr_t func) {
    onPatternMessageCallbacks.add(obj, func);
  }
  void removeOnPatternMessageCallback(SysexCallback *obj,
                                      sysex_callback_ptr_t func) {
    onPatternMessageCallbacks.remove(obj, func);
  }
  void removeOnPatternMessageCallback(SysexCallback *obj) {
    onPatternMessageCallbacks.remove(obj);
  }

  void addOnSongMessageCallback(SysexCallback *obj, sysex_callback_ptr_t func) {
    onSongMessageCallbacks.add(obj, func);
  }
  void removeOnSongMessageCallback(SysexCallback *obj, sysex_callback_ptr_t func) {
    onSongMessageCallbacks.remove(obj, func);
  }
  void removeOnSongMessageCallback(SysexCallback *obj) {
    onSongMessageCallbacks.remove(obj);
  }
};

extern MNMSysexListenerClass MNMSysexListener;

#endif /* MNM_SYSEX_H__ */
