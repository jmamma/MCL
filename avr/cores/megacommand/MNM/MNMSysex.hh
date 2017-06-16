#ifndef MNM_SYSEX_H__
#define MNM_SYSEX_H__

#include "WProgram.h"
#include "Midi.h"
#include "MidiSysex.hh"
#include "Vector.hh"
#include "Elektron.hh"
#include "Circular.hh"

typedef void(MNMCallback::*mnm_callback_ptr_t)();
typedef void(MNMCallback::*mnm_status_callback_ptr_t)(uint8_t p1, uint8_t p2);


class MNMSysexListenerClass : public MidiSysexListenerClass {
public:
  CallbackVector<MNMCallback,8> onGlobalMessageCallbacks;
  CallbackVector<MNMCallback,8> onKitMessageCallbacks;
  CallbackVector<MNMCallback,8> onSongMessageCallbacks;
  CallbackVector<MNMCallback,8> onPatternMessageCallbacks;

  CallbackVector2<MNMCallback,8,uint8_t,uint8_t> onStatusResponseCallbacks;
  
  bool isMNMMessage;
  bool isMNMEncodedMessage;
  uint8_t msgType;

  CircularBuffer<uint8_t, 4> sysexCirc;
  MNMSysexToDataEncoder encoder;
  uint16_t msgLen;
  uint16_t msgCksum;
  
  MNMSysexListenerClass() : MidiSysexListenerClass() {
    ids[0] = 0;
    ids[1] = 0x20;
    ids[2] = 0x3c;
  }
  
  virtual void start();
  virtual void handleByte(uint8_t byte);
  virtual void end();

  void setup();

  void addOnStatusResponseCallback(MNMCallback *obj, mnm_status_callback_ptr_t func) {
    onStatusResponseCallbacks.add(obj, func);
  }
  void removeOnStatusResponseCallback(MNMCallback *obj, mnm_status_callback_ptr_t func) {
    onStatusResponseCallbacks.remove(obj, func);
  }
  void removeOnStatusResponseCallback(MNMCallback *obj) {
    onStatusResponseCallbacks.remove(obj);
  }

  void addOnGlobalMessageCallback(MNMCallback *obj, mnm_callback_ptr_t func) {
    onGlobalMessageCallbacks.add(obj, func);
  }
  void removeOnGlobalMessageCallback(MNMCallback *obj, mnm_callback_ptr_t func) {
    onGlobalMessageCallbacks.remove(obj, func);
  }
  void removeOnGlobalMessageCallback(MNMCallback *obj) {
    onGlobalMessageCallbacks.remove(obj);
  }
  
  void addOnKitMessageCallback(MNMCallback *obj, mnm_callback_ptr_t func) {
    onKitMessageCallbacks.add(obj, func);
  }
  void removeOnKitMessageCallback(MNMCallback *obj, mnm_callback_ptr_t func) {
    onKitMessageCallbacks.remove(obj, func);
  }
  void removeOnKitMessageCallback(MNMCallback *obj) {
    onKitMessageCallbacks.remove(obj);
  }
  
  void addOnPatternMessageCallback(MNMCallback *obj, mnm_callback_ptr_t func) {
    onPatternMessageCallbacks.add(obj, func);
  }
  void removeOnPatternMessageCallback(MNMCallback *obj, mnm_callback_ptr_t func) {
    onPatternMessageCallbacks.remove(obj, func);
  }
  void removeOnPatternMessageCallback(MNMCallback *obj) {
    onPatternMessageCallbacks.remove(obj);
  }
  
  void addOnSongMessageCallback(MNMCallback *obj, mnm_callback_ptr_t func) {
    onSongMessageCallbacks.add(obj, func);
  }
  void removeOnSongMessageCallback(MNMCallback *obj, mnm_callback_ptr_t func) {
    onSongMessageCallbacks.remove(obj, func);
  }
  void removeOnSongMessageCallback(MNMCallback *obj) {
    onSongMessageCallbacks.remove(obj);
  }
};

extern MNMSysexListenerClass MNMSysexListener;

#endif /* MNM_SYSEX_H__ */
