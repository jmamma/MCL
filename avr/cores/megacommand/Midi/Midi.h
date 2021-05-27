/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MIDI_H__
#define MIDI_H__

#include <stdlib.h>

#include <inttypes.h>

// #include "MidiSDS.h"
#include "Callback.h"
#include "ListPool.h"
#include "Vector.h"

class MidiUartParent;

extern "C" {
#include "midi-common.h"
}

/**
 * \addtogroup Midi
 *
 * @{
 **/

/**
 * \addtogroup midi_class Midi Parser Class
 *
 * @{
 **/

#define MIDI_NOTE_OFF_CB 0
#define MIDI_NOTE_ON_CB 1
#define MIDI_AT_CB 2
#define MIDI_CC_CB 3
#define MIDI_PRG_CHG_CB 4
#define MIDI_CHAN_PRESS_CB 5
#define MIDI_PITCH_WHEEL_CB 6

typedef struct {
  uint8_t midi_status;
  midi_state_t next_state;
} midi_parse_t;

class MidiSysexClass;

typedef void (MidiCallback::*midi_callback_ptr_t)(uint8_t *msg);
typedef void (MidiCallback::*midi_callback_ptr2_t)(uint8_t *msg, uint8_t len);

#include "MidiSysex.h"

class MidiClass {
  /**
   * \addtogroup midi_class
   *
   * @{
   **/

public:
  midi_state_t in_state;
  midi_state_t live_state; // state used for MIDI messages received on UART (not
                           // processed by loop)
  uint8_t last_status;
  uint8_t running_status;
  uint8_t in_msg_len;
  uint8_t msg[3];

  MidiUartParent *uart;
  MidiUartClassCommon *uart_forward;
  uint8_t callback;
  //  midi_callback_t callbacks[7];
  CallbackVector1<MidiCallback, 8, uint8_t *> midiCallbacks[7];
#ifdef HOST_MIDIDUINO
  CallbackVector2<MidiCallback, 8, uint8_t *, uint8_t> messageCallback;
#endif

  bool midiActive;
  MidiSysexClass midiSysex;
  uint8_t receiveChannel;

  MidiClass(MidiUartParent *_uart, uint16_t _sysexBufLen, volatile uint8_t *ptr);

  void init();
  void sysexEnd(uint8_t msg_rd);
  void handleByte(uint8_t c);

#ifdef HOST_MIDIDUINO
  void addOnMessageCallback(MidiCallback *obj,
                            void (MidiCallback::*func)(uint8_t *msg,
                                                       uint8_t len)) {
    messageCallback.add(obj, func);
  }
  void removeOnMessageCallback(MidiCallback *obj,
                               void (MidiCallback::*func)(uint8_t *msg,
                                                          uint8_t len)) {
    messageCallback.remove(obj, func);
  }
  void removeOnMessageCallback(MidiCallback *obj) {
    messageCallback.remove(obj);
  }
#endif

  void addOnControlChangeCallback(MidiCallback *obj,
                                  void (MidiCallback::*func)(uint8_t *msg)) {
    midiCallbacks[MIDI_CC_CB].add(obj, func);
  }
  void removeOnControlChangeCallback(MidiCallback *obj,
                                     void (MidiCallback::*func)(uint8_t *msg)) {
    midiCallbacks[MIDI_CC_CB].remove(obj, func);
  }
  void removeOnControlChangeCallback(MidiCallback *obj) {
    midiCallbacks[MIDI_CC_CB].remove(obj);
  }

  void addOnNoteOnCallback(MidiCallback *obj,
                           void (MidiCallback::*func)(uint8_t *msg)) {
    midiCallbacks[MIDI_NOTE_ON_CB].add(obj, func);
  }
  void removeOnNoteOnCallback(MidiCallback *obj,
                              void (MidiCallback::*func)(uint8_t *msg)) {
    midiCallbacks[MIDI_NOTE_ON_CB].remove(obj, func);
  }
  void removeOnNoteOnCallback(MidiCallback *obj) {
    midiCallbacks[MIDI_NOTE_ON_CB].remove(obj);
  }

  void addOnNoteOffCallback(MidiCallback *obj,
                            void (MidiCallback::*func)(uint8_t *msg)) {
    midiCallbacks[MIDI_NOTE_OFF_CB].add(obj, func);
  }
  void removeOnNoteOffCallback(MidiCallback *obj,
                               void (MidiCallback::*func)(uint8_t *msg)) {
    midiCallbacks[MIDI_NOTE_OFF_CB].remove(obj, func);
  }
  void removeOnNoteOffCallback(MidiCallback *obj) {
    midiCallbacks[MIDI_NOTE_OFF_CB].remove(obj);
  }

  void addOnAfterTouchCallback(MidiCallback *obj,
                               void (MidiCallback::*func)(uint8_t *msg)) {
    midiCallbacks[MIDI_AT_CB].add(obj, func);
  }
  void removeOnAfterTouchCallback(MidiCallback *obj,
                                  void (MidiCallback::*func)(uint8_t *msg)) {
    midiCallbacks[MIDI_AT_CB].remove(obj, func);
  }
  void removeOnAfterTouchCallback(MidiCallback *obj) {
    midiCallbacks[MIDI_AT_CB].remove(obj);
  }

  void addOnProgramChangeCallback(MidiCallback *obj,
                                  void (MidiCallback::*func)(uint8_t *msg)) {
    midiCallbacks[MIDI_PRG_CHG_CB].add(obj, func);
  }
  void removeOnProgramChangeCallback(MidiCallback *obj,
                                     void (MidiCallback::*func)(uint8_t *msg)) {
    midiCallbacks[MIDI_PRG_CHG_CB].remove(obj, func);
  }
  void removeOnProgramChangeCallback(MidiCallback *obj) {
    midiCallbacks[MIDI_PRG_CHG_CB].remove(obj);
  }

  void addOnChannelPressureCallback(MidiCallback *obj,
                                    void (MidiCallback::*func)(uint8_t *msg)) {
    midiCallbacks[MIDI_CHAN_PRESS_CB].add(obj, func);
  }
  void
  removeOnChannelPressureCallback(MidiCallback *obj,
                                  void (MidiCallback::*func)(uint8_t *msg)) {
    midiCallbacks[MIDI_CHAN_PRESS_CB].remove(obj, func);
  }
  void removeOnChannelPressureCallback(MidiCallback *obj) {
    midiCallbacks[MIDI_CHAN_PRESS_CB].remove(obj);
  }

  void addOnPitchWheelCallback(MidiCallback *obj,
                               void (MidiCallback::*func)(uint8_t *msg)) {
    midiCallbacks[MIDI_PITCH_WHEEL_CB].add(obj, func);
  }
  void removeOnPitchWheelCallback(MidiCallback *obj,
                                  void (MidiCallback::*func)(uint8_t *msg)) {
    midiCallbacks[MIDI_PITCH_WHEEL_CB].remove(obj, func);
  }
  void removeOnPitchWheelCallback(MidiCallback *obj) {
    midiCallbacks[MIDI_PITCH_WHEEL_CB].remove(obj);
  }

  /* @} */
};

extern MidiClass Midi;
extern MidiClass Midi2;
extern MidiClass USBMidi;

/* @} @} */

#include <MidiUartParent.h>

#endif /* MIDI_H__ */
