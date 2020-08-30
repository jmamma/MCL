
#ifndef A4SYSEX_H__
#define A4SYSEX_H__

#include "A4.h"
#include "Callback.h"
#include "Midi.h"
#include "MidiSysex.h"
#include "Vector.h"
#include "WProgram.h"

class A4SysexListenerClass : public MidiSysexListenerClass {
  /**
   * \addtogroup A4_sysex_listener
   *
   * @{
   **/

public:
  /** Vector storing the onGlobalMessage callbacks (called when a global message
   * is received). **/
  CallbackVector<SysexCallback, 8> onGlobalMessageCallbacks;
  /** Vector storing the onKitMessage callbacks (called when a kit message is
   * received). **/
  CallbackVector<SysexCallback, 8> onKitMessageCallbacks;
  /** Vector storing the onSongMessage callbacks (called when a song messages is
   * received). **/
  CallbackVector<SysexCallback, 8> onSongMessageCallbacks;
  /** Vector storing the onPatternMessage callbacks (called when a pattern
   * message is received). **/
  CallbackVector<SysexCallback, 8> onPatternMessageCallbacks;

  CallbackVector<SysexCallback, 8> onSoundMessageCallbacks;

  CallbackVector<SysexCallback, 8> onSettingsMessageCallbacks;

  /** Stores if the currently received message is a MachineDrum sysex message.
   * **/
  bool isA4Message;
  /** Stores the message type of the currently received sysex message. **/
  uint8_t msgType;

  A4SysexListenerClass() : MidiSysexListenerClass() {
    ids[0] = 0;
    ids[1] = 0x20;
    ids[2] = 0x3c;
  }

  virtual void start();
  virtual void handleByte(uint8_t byte);
  virtual void end_immediate();
  virtual void end();
  /**
   * Add the sysex listener to the MIDI sysex subsystem. This needs to
   * be called if you want to use the A4SysexListener (it is called
   * automatically by the A4Task subsystem though).
   **/
  void setup(MidiClass *_midi);

  void addOnSoundMessageCallback(SysexCallback *obj, sysex_callback_ptr_t func) {
    onSoundMessageCallbacks.add(obj, func);
  }
  void removeOnSoundMessageCallback(SysexCallback *obj, sysex_callback_ptr_t func) {
    onSoundMessageCallbacks.remove(obj, func);
  }
  void removeOnSoundMessageCallback(SysexCallback *obj) {
    onSoundMessageCallbacks.remove(obj);
  }

  void addOnSettingsMessageCallback(SysexCallback *obj, sysex_callback_ptr_t func) {
    onSettingsMessageCallbacks.add(obj, func);
  }
  void removeOnSettingsMessageCallback(SysexCallback *obj,
                                       sysex_callback_ptr_t func) {
    onSettingsMessageCallbacks.remove(obj, func);
  }
  void removeOnSettingsMessageCallback(SysexCallback *obj) {
    onSettingsMessageCallbacks.remove(obj);
  }

  void addOnGlobalMessageCallback(SysexCallback *obj, sysex_callback_ptr_t func) {
    onGlobalMessageCallbacks.add(obj, func);
  }
  void removeOnGlobalMessageCallback(SysexCallback *obj, sysex_callback_ptr_t func) {
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
  void removeOnPatternMessageCallback(SysexCallback *obj, sysex_callback_ptr_t func) {
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

  /* @} */
};

#include "A4Messages.h"

extern A4SysexListenerClass A4SysexListener;

/* @} @} */

#endif /* A4SYSEX_H__ */
