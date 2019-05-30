/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MIDIUARTPARENT_H__
#define MIDIUARTPARENT_H__

#include "Callback.hh"
#include "Vector.hh"
#include <midi-common.hh>
#include "MidiID.hh"

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

class MidiUartParent {
  /**
   * \addtogroup midi_uart
   *
   * @{
   **/

public:
  uint8_t running_status;
  uint8_t currentChannel;
  uint8_t uart_port;
  uint8_t uart_block;
  uint32_t speed;

  bool useRunningStatus;
  uint16_t sendActiveSenseTimer;
  uint16_t sendActiveSenseTimeout;
  uint16_t recvActiveSenseTimer;
  bool activeSenseEnabled;

  MidiID device;

  MidiUartParent() {
    useRunningStatus = false;
    running_status = 0;
    currentChannel = 0x0;
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

  inline void tickActiveSense() {
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

  virtual void initSerial() { running_status = 0; }

  virtual void puts(uint8_t *data, uint16_t cnt) {
    while (cnt--)
      m_putc(*(data++));
  }
  virtual uint8_t m_getc() {}
  virtual void m_putc(uint8_t c) {}
  virtual void m_putc_immediate(uint8_t c) { m_putc(c); }
  virtual bool avail() { return false; }

  virtual uint8_t getc() { return 0; }

  inline virtual void sendMessage(uint8_t cmdByte) { sendCommandByte(cmdByte); }
  inline virtual void sendMessage(uint8_t cmdByte, uint8_t byte1) {
    uart_block = 1;
    sendCommandByte(cmdByte);
    m_putc(byte1);
    uart_block = 0;
  }

  inline virtual void sendMessage(uint8_t cmdByte, uint8_t byte1, uint8_t byte2) {
    uart_block = 1;
    sendCommandByte(cmdByte);
    m_putc(byte1);
    m_putc(byte2);
    uart_block = 0;
  }

  inline void sendCommandByte(uint8_t byte) {
   #ifdef MIDI_RUNNING_STATUS
    if (MIDI_IS_REALTIME_STATUS_BYTE(byte) ||
        MIDI_IS_SYSCOMMON_STATUS_BYTE(byte)) {
      if (!MIDI_IS_REALTIME_STATUS_BYTE(byte)) {
        running_status = 0;
        m_putc(byte);
      } else {
        m_putc_immediate(byte);
      }
    } else {
      if (useRunningStatus) {
        if (running_status != byte)
          m_putc(byte);
        running_status = byte;
      } else {
        m_putc(byte);
      }
    }
   #else
    m_putc(byte);
   #endif
  }

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

  inline void resetRunningStatus() { running_status = 0; }

  inline void sendNoteOn(uint8_t note, uint8_t velocity) {
    sendNoteOn(currentChannel, note, velocity);
  }
  inline void sendNoteOff(uint8_t note, uint8_t velocity) {
    sendNoteOff(currentChannel, note, velocity);
  }
  inline void sendNoteOff(uint8_t note) {
    sendNoteOff(currentChannel, note, 0);
  }
  inline void sendCC(uint8_t cc, uint8_t value) {
    sendCC(currentChannel, cc, value);
  }
  inline void sendProgramChange(uint8_t program) {
    sendProgramChange(currentChannel, program);
  }

  inline void sendPolyKeyPressure(uint8_t note, uint8_t pressure) {
    sendPolyKeyPressure(currentChannel, note, pressure);
  }

  inline void sendChannelPressure(uint8_t pressure) {
    sendChannelPressure(currentChannel, pressure);
  }

  inline void sendPitchBend(int16_t pitchbend) {
    sendPitchBend(currentChannel, pitchbend);
  }

  inline void sendNRPN(uint16_t parameter, uint8_t value) {
    sendNRPN(currentChannel, parameter, value);
  }
  inline void sendNRPN(uint16_t parameter, uint16_t value) {
    sendNRPN(currentChannel, parameter, value);
  }

  inline void sendRPN(uint16_t parameter, uint8_t value) {
    sendRPN(currentChannel, parameter, value);
  }
  inline void sendRPN(uint16_t parameter, uint16_t value) {
    sendRPN(currentChannel, parameter, value);
  }

  inline void sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    #ifdef MIDI_VALIDATE
    if ((channel >= 16) || (note >= 128) || (velocity >= 128))
      return;
    #endif

    uint8_t msg[3] = {MIDI_NOTE_ON | channel, note, velocity};
    //noteOnCallbacks.call(msg);
    sendMessage(msg[0], msg[1], msg[2]);
  }

  inline void sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
    #ifdef MIDI_VALIDATE
    if ((channel >= 16) || (note >= 128) || (velocity >= 128))
      return;
    #endif

    uint8_t msg[3] = {MIDI_NOTE_OFF | channel, note, velocity};
    //noteOffCallbacks.call(msg);
    sendMessage(msg[0], msg[1], msg[2]);
  }

  inline void sendCC(uint8_t channel, uint8_t cc, uint8_t value) {
    #ifdef MIDI_VALIDATE
    if ((channel >= 16) || (note >= 128) || (velocity >= 128))
      return;
    #endif

    uint8_t msg[3] = {MIDI_CONTROL_CHANGE | channel, cc, value};
    //ccCallbacks.call(msg);
    sendMessage(msg[0], msg[1], msg[2]);
  }

  inline void sendProgramChange(uint8_t channel, uint8_t program) {
    #ifdef MIDI_VALIDATE
    if ((channel >= 16) || (note >= 128) || (velocity >= 128))
      return;
    #endif

    sendMessage(MIDI_PROGRAM_CHANGE | channel, program);
  }

  void sendPolyKeyPressure(uint8_t channel, uint8_t note, uint8_t pressure) {
    #ifdef MIDI_VALIDATE
    if ((channel >= 16) || (note >= 128) || (velocity >= 128))
      return;
    #endif

    sendMessage(MIDI_AFTER_TOUCH | channel, note, pressure);
  }

  void sendChannelPressure(uint8_t channel, uint8_t pressure) {
    #ifdef MIDI_VALIDATE
    if ((channel >= 16) || (note >= 128) || (velocity >= 128))
      return;
    #endif

   sendMessage(MIDI_CHANNEL_PRESSURE | channel, pressure);
  }

  void sendPitchBend(uint8_t channel, int16_t pitchbend) {
    pitchbend += 8192;
    sendMessage(MIDI_PITCH_WHEEL | channel, pitchbend & 0x7F,
                (pitchbend >> 7) & 0x7F);
  }

  void sendNRPN(uint8_t channel, uint16_t parameter, uint8_t value) {
    sendCC(channel, 99, (parameter >> 7) & 0x7F);
    sendCC(channel, 98, (parameter & 0x7F));
    sendCC(channel, 6, value);
  }
  void sendNRPN(uint8_t channel, uint16_t parameter, uint16_t value) {
    sendCC(channel, 99, (parameter >> 7) & 0x7F);
    sendCC(channel, 98, (parameter & 0x7F));
    sendCC(channel, 6, (value >> 7) & 0x7F);
    sendCC(channel, 38, (value & 0x7F));
  }

  void sendRPN(uint8_t channel, uint16_t parameter, uint8_t value) {
    sendCC(channel, 101, (parameter >> 7) & 0x7F);
    sendCC(channel, 100, (parameter & 0x7F));
    sendCC(channel, 6, value);
  }
  void sendRPN(uint8_t channel, uint16_t parameter, uint16_t value) {
    sendCC(channel, 101, (parameter >> 7) & 0x7F);
    sendCC(channel, 100, (parameter & 0x7F));
    sendCC(channel, 6, (value >> 7) & 0x7F);
    sendCC(channel, 38, (value & 0x7F));
  }

  virtual void sendSysex(uint8_t *data, uint8_t cnt) {
    sendCommandByte(0xF0);
    puts(data, cnt);
    sendCommandByte(0xF7);
  }
  inline void sendRaw(uint8_t *msg, uint16_t cnt) { puts(msg, cnt); }
  inline void sendRaw(uint8_t byte) { m_putc(byte); }

  void sendString(const char *data) { sendString(data, m_strlen(data)); }
  void sendString(const char *data, uint16_t cnt);

  void printfString(const char *fmt, ...) {
    va_list lp;
    va_start(lp, fmt);
    char buf[128];
    uint16_t len = m_vsnprintf(buf, sizeof(buf), fmt, lp);
    va_end(lp);
    sendString(buf, len);
  }

#ifdef HOST_MIDIDUINO
  virtual ~MidiUartParent() {}
#endif

  /* @} */
};

#endif /* MIDIUARTPARENT_H__ */
