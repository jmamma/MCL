#include "WProgram.h"

#include "Midi.h"
#include "MidiSDS.hh"

void MidiSDSClass::sendGeneralMessage(uint8_t type) {
  uint8_t data[6] = {
    0xF0, 0x7E, 0x00, 0x00, 0x00, 0xF7
  };
  data[2] = deviceID;
  data[3] = type;
  data[4] = packetNumber;
  MidiUart.sendRaw(data, 6);
}

void MidiSDSClass::sendAckMessage() {
  sendGeneralMessage(0x7F);
}

void MidiSDSClass::sendNakMessage() {
  sendGeneralMessage(0x7E);
}

void MidiSDSClass::sendCancelMessage() {
  sendGeneralMessage(0x7D);
}

void MidiSDSClass::sendWaitMessage() {
  sendGeneralMessage(0x7C);
}

void MidiSDSClass::sendEOFMessage() {
  sendGeneralMessage(0x7B);
}

void MidiSDSClass::sendDumpRequest() {
  uint8_t data[7] = {
    0xF0, 0x7E, 0x00, 0x03, 0x00, 0x00, 0xF7
  };
  data[2] = deviceID;
  data[4] = sampleNumber & 0x7F;
  data[5] = (sampleNumber >> 7) & 0x7F;
  MidiUart.sendRaw(data, 7);
}

void MidiSDSClass::sendDumpHeader() {
  uint8_t data[21] = {
    0xF0, 0x7E, 0x00, 0x1,
    0x00, 0x00,
    0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00,
    0xF7};
  data[2] = deviceID;
  data[4] = (sampleNumber & 0x7F);
  data[5] = (sampleNumber >> 7) & 0x7F;
  data[6] = sampleFormat;

  data[7] = (samplePeriod) & 0x7F;
  data[8] = (samplePeriod >> 7) & 0x7F;
  data[9] = (samplePeriod >> 14) & 0x7F;

  data[10] = (sampleLength) & 0x7F;
  data[11] = (sampleLength >> 7) & 0x7F;
  data[12] = (sampleLength >> 14) & 0x7F;

  data[13] = (loopStart) & 0x7F;
  data[14] = (loopStart >> 7) & 0x7F;
  data[15] = (loopStart >> 14) & 0x7F;

  data[16] = (loopEnd) & 0x7F;
  data[17] = (loopEnd >> 7) & 0x7F;
  data[18] = (loopEnd >> 14) & 0x7F;

  data[19] = loopType;

  MidiUart.sendRaw(data, 21);
}

bool MidiSDSClass::sendData(uint8_t *buf, uint8_t len) {
  if (len > 120)
    return false;
  uint8_t data[5] = {
    0xF0, 0x7E, 0x00, 0x02, 0x00
  };
  data[2] = deviceID;
  data[4] = packetNumber;
  uint8_t checksum = 0;
  MidiUart.sendRaw(data, 5);
  for (int i = 1; i < 5; i++)
    checksum ^= data[i];
  MidiUart.sendRaw(buf, len);
  for (int i = 0; i < len; i++)
    checksum ^= buf[i];
  for (int i = len; i < 120; i++)
    MidiUart.m_putc(0x00);
  MidiUart.m_putc(checksum & 0x7F);
  MidiUart.m_putc(0xF7);
  return true;
}
