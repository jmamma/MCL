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
class MDSysexListenerClass : public ElektronSysexListenerClass {
  /**
   * \addtogroup md_sysex_listener
   *
   * @{
   **/

public:
  /** Stores if the currently received message is a MachineDrum sysex message.
   * **/
  bool isMDMessage;

  MDSysexListenerClass() : ElektronSysexListenerClass() {
    ids[0] = 0;
    ids[1] = 0x20;
    ids[2] = 0x3c;
  }

  virtual void start();
  virtual void handleByte(uint8_t byte);
  virtual void end();

  /**
   * Add the sysex listener to the MIDI sysex subsystem. This needs to
   * be called if you want to use the MDSysexListener (it is called
   * automatically by the MDTask subsystem though).
   **/
  void setup(MidiClass *_midi);

    /* @} */
};

#include "MDMessages.h"

extern MDSysexListenerClass MDSysexListener;

/* @} @} */

#endif /* MDSYSEX_H__ */
