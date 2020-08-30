/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MDSYSEX_H__
#define MDSYSEX_H__

#include "Callback.h"
#include "MD.h"
#include "Midi.h"
#include "MidiSysex.h"
#include "Vector.h"
#include "WProgram.h"

/**
 * \addtogroup MD Elektron MachineDrum
 *
 * @{
 *
 * \addtogroup md_sysex MachineDrum Sysex Messages
 *
 * @{
 **/

typedef enum {
  MD_NONE,

  MD_GET_CURRENT_KIT,
  MD_GET_KIT,

  MD_GET_CURRENT_GLOBAL,
  MD_GET_GLOBAL,

  MD_DONE
} getCurrentKitStatus_t;

/**
 * \addtogroup md_sysex_listener MachineDrum Sysex Listener
 *
 * @{
 **/

/**
 * This class is the sysex listener for the machinedrum, interpreting
 * received sysex messages from the machinedrum, and dispatching it to
 * callbacks.
 **/
class MDSysexListenerClass : public MidiSysexListenerClass {
  /**
   * \addtogroup md_sysex_listener
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
  /** Vector storing the onStatusResponse callbacks (when a status response is
   * received). **/
  CallbackVector2<SysexCallback, 8, uint8_t, uint8_t> onStatusResponseCallbacks;

  CallbackVector<SysexCallback, 1> onSampleNameCallbacks;
  /** Stores if the currently received message is a MachineDrum sysex message.
   * **/
  bool isMDMessage;
  /** Stores the message type of the currently received sysex message. **/
  uint8_t msgType;

  MDSysexListenerClass() : MidiSysexListenerClass() {
    ids[0] = 0;
    ids[1] = 0x20;
    ids[2] = 0x3c;
  }

  virtual void start();
  virtual void handleByte(uint8_t byte);
  virtual void end();
  virtual void end_immediate();

  /**
   * Add the sysex listener to the MIDI sysex subsystem. This needs to
   * be called if you want to use the MDSysexListener (it is called
   * automatically by the MDTask subsystem though).
   **/
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

  void addOnSampleNameCallback(SysexCallback *obj, sysex_callback_ptr_t func) {
    onSampleNameCallbacks.add(obj, func);
  }
  void removeOnSampleNameCallback(SysexCallback *obj, sysex_callback_ptr_t func) {
    onSampleNameCallbacks.remove(obj, func);
  }
  void removeOnSampleNameCallback(SysexCallback *obj) {
    onSampleNameCallbacks.remove(obj);
  }

  /* @} */
};

#include "MDMessages.h"

extern MDSysexListenerClass MDSysexListener;

/* @} @} */

#endif /* MDSYSEX_H__ */
