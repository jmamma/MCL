/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MIDIUARTPARENT_H__
#define MIDIUARTPARENT_H__

#include "Callback.h"
#include "Core.h"
#include "MidiID.h"
#include "Vector.h"
#include <midi-common.h>
//#define MIDI_VALIDATE
//#define MIDI_RUNNING_STATUS

/**
 * \addtogroup Midi
 *
 * @{
 **/

/**
 * \addtogroup midi_uart Midi UART Parent Class
 *
 * @{
 **/

class MidiClass;

class MidiUartParent {
  /**
   * \addtogroup midi_uart
   *
   * @{
   **/

public:
  volatile static uint8_t handle_midi_lock;

  uint32_t speed;

  uint16_t sendActiveSenseTimer;
  uint16_t sendActiveSenseTimeout;
  uint16_t recvActiveSenseTimer;
  bool activeSenseEnabled;

  uint8_t mode;

  MidiClass *midi;

  MidiID device;

  MidiUartParent() {
    activeSenseEnabled = 0;
    recvActiveSenseTimer = 0;
    sendActiveSenseTimer = 0;
  }

  void setActiveSenseTimer(uint16_t timeout) {
    if (timeout == 0) {
      activeSenseEnabled = false;
    } else {
      activeSenseEnabled = true;
      sendActiveSenseTimer = 0;
      sendActiveSenseTimeout = timeout;
    }
  }

  ALWAYS_INLINE() void tickActiveSense() {
    if (recvActiveSenseTimer < 65535) {
      recvActiveSenseTimer++;
    }
    if (activeSenseEnabled) {
      if (sendActiveSenseTimer == 0) {
        m_putc(MIDI_ACTIVE_SENSE);
        sendActiveSenseTimer = sendActiveSenseTimeout;
      } else {
        sendActiveSenseTimer--;
      }
    }
  }
  virtual void initSerial() { }

  virtual uint8_t m_getc() = 0;
  virtual void m_putc(uint8_t *src, uint16_t size) = 0;
  virtual void m_putc(uint8_t c) = 0;
  virtual void m_putc_immediate(uint8_t c) { m_putc(c); }
  virtual void m_recv(uint8_t *src, uint16_t size) = 0;
  virtual bool avail() { return false; }

  virtual uint8_t getc() { return 0; }

  ALWAYS_INLINE() virtual void sendMessage(uint8_t cmdByte) { m_putc(cmdByte); }
  ALWAYS_INLINE() virtual void sendMessage(uint8_t cmdByte, uint8_t byte1) {
    uint8_t data[2] = { cmdByte, (uint8_t)(byte1 & 0x7F) };
    m_putc(data,2);
  }

  ALWAYS_INLINE() virtual void sendMessage(uint8_t cmdByte, uint8_t byte1, uint8_t byte2) {
    uint8_t data[3] = { cmdByte, (uint8_t)(byte1 & 0x7F), (uint8_t)(byte2 & 0x7F) };
    m_putc(data,3);
  }

  ALWAYS_INLINE() void sendCommandByte(uint8_t byte) {
    m_putc(byte);
  }
  /*
  CallbackVector1<MidiCallback, 8, uint8_t *> noteOnCallbacks;
  CallbackVector1<MidiCallback, 8, uint8_t *> noteOffCallbacks;
  CallbackVector1<MidiCallback, 8, uint8_t *> ccCallbacks;
  CallbackVector1<MidiCallback, 8, uint8_t *> recvActiveSenseCallbacks;

  void addOnRecvActiveSenseCallback(MidiCallback *obj,
                                    void (MidiCallback::*func)(uint8_t *msg)) {
    recvActiveSenseCallbacks.add(obj, func);
  }
  void
  removeOnRecvActiveSenseCallback(MidiCallback *obj,
                                  void (MidiCallback::*func)(uint8_t *msg)) {
    recvActiveSenseCallbacks.remove(obj, func);
  }
  void removeOnRecvActiveSenseCallback(MidiCallback *obj) {
    recvActiveSenseCallbacks.remove(obj);
  }

  void addOnNoteOnCallback(MidiCallback *obj,
                           void (MidiCallback::*func)(uint8_t *msg)) {
    noteOnCallbacks.add(obj, func);
  }
  void removeOnNoteOnCallback(MidiCallback *obj,
                              void (MidiCallback::*func)(uint8_t *msg)) {
    noteOnCallbacks.remove(obj, func);
  }
  void removeOnNoteOnCallback(MidiCallback *obj) {
    noteOnCallbacks.remove(obj);
  }

  void addOnNoteOffCallback(MidiCallback *obj,
                            void (MidiCallback::*func)(uint8_t *msg)) {
    noteOffCallbacks.add(obj, func);
  }
  void removeOnNoteOffCallback(MidiCallback *obj,
                               void (MidiCallback::*func)(uint8_t *msg)) {
    noteOffCallbacks.remove(obj, func);
  }
  void removeOnNoteOffCallback(MidiCallback *obj) {
    noteOffCallbacks.remove(obj);
  }

  void addOnControlChangeCallback(MidiCallback *obj,
                                  void (MidiCallback::*func)(uint8_t *msg)) {
    ccCallbacks.add(obj, func);
  }
  void removeOnControlChangeCallback(MidiCallback *obj,
                                     void (MidiCallback::*func)(uint8_t *msg)) {
    ccCallbacks.remove(obj, func);
  }
  void removeOnControlChangeCallback(MidiCallback *obj) {
    ccCallbacks.remove(obj);
  }
  */

  void sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    if (channel >= 16)
      return;
    uint8_t msg[3] = {(uint8_t)(MIDI_NOTE_ON | channel), note, velocity};
    // noteOnCallbacks.call(msg);
    sendMessage(msg[0], msg[1], msg[2]);
  }

  void sendNoteOff(uint8_t channel, uint8_t note) {
    if (channel >= 16)
      return;
    //uint8_t msg[3] = {(uint8_t)(MIDI_NOTE_OFF | channel), note, velocity};
    //Send Note On VEL = 0, to leverage running status
    //
    uint8_t msg[3] = {(uint8_t)(MIDI_NOTE_ON | channel), note, 0};
    // noteOffCallbacks.call(msg);
    sendMessage(msg[0], msg[1], msg[2]);
  }

  void sendCC(uint8_t channel, uint8_t cc, uint8_t value) {
    if (channel >= 16)
      return;
    uint8_t msg[3] = {(uint8_t)(MIDI_CONTROL_CHANGE | channel), cc, value};
    // ccCallbacks.call(msg);
    sendMessage(msg[0], msg[1], msg[2]);
  }

  ALWAYS_INLINE() void sendProgramChange(uint8_t channel, uint8_t program) {
    if (channel >= 16)
      return;
    sendMessage((uint8_t)(MIDI_PROGRAM_CHANGE | channel), program);
  }

  void sendPolyKeyPressure(uint8_t channel, uint8_t note, uint8_t pressure) {
    if (channel >= 16)
      return;
    sendMessage((uint8_t)(MIDI_AFTER_TOUCH | channel), note, pressure);
  }

  void sendChannelPressure(uint8_t channel, uint8_t pressure) {
    if (channel >= 16)
      return;
    sendMessage((uint8_t)(MIDI_CHANNEL_PRESSURE | channel), pressure);
  }

  void sendPitchBend(uint8_t channel, int16_t pitchbend) {
    if (channel >= 16)
      return;
    sendMessage((uint8_t)(MIDI_PITCH_WHEEL | channel), pitchbend,
                (pitchbend >> 7));
  }

  void sendNRPN(uint8_t channel, uint16_t parameter, uint8_t value) {
    if (channel >= 16)
      return;
    sendCC(channel, 99, (parameter >> 7));
    sendCC(channel, 98, (parameter));
    sendCC(channel, 6, value);
  }
  void sendNRPN(uint8_t channel, uint16_t parameter, uint16_t value) {
    if (channel >= 16)
      return;
    sendCC(channel, 99, (parameter >> 7));
    sendCC(channel, 98, (parameter));
    sendCC(channel, 6, (value >> 7));
    sendCC(channel, 38, (value));
  }

  void sendRPN(uint8_t channel, uint16_t parameter, uint8_t value) {
    if (channel >= 16)
      return;
    sendCC(channel, 101, (parameter >> 7));
    sendCC(channel, 100, (parameter));
    sendCC(channel, 6, value);
  }
  void sendRPN(uint8_t channel, uint16_t parameter, uint16_t value) {
     if (channel >= 16)
      return;
    sendCC(channel, 101, (parameter >> 7));
    sendCC(channel, 100, (parameter));
    sendCC(channel, 6, (value >> 7));
    sendCC(channel, 38, (value));
  }

  virtual void sendSysex(uint8_t *data, uint8_t cnt) {
    sendCommandByte(0xF0);
    m_putc(data, cnt);
    sendCommandByte(0xF7);
  }
  void sendRaw(uint8_t *msg, uint16_t cnt) { m_putc(msg, cnt); }
  void sendRaw(uint8_t byte) { m_putc(byte); }

  void sendString(const char *data) { sendString(data, strlen(data)); }
  void sendString(const char *data, uint16_t cnt);

  void printfString(char *fmt, ...) {
    va_list lp;
    va_start(lp, fmt);
    char buf[128];
    uint16_t len = vsnprintf(buf, sizeof(buf), fmt, lp);
    va_end(lp);
    sendString(buf, len);
  }

#ifdef HOST_MIDIDUINO
  virtual ~MidiUartParent() {}
#endif

  /* @} */
};

#endif /* MIDIUARTPARENT_H__ */
