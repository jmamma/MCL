#ifndef MIDISDS_H__
#define MIDISDS_H__

#include "Wav.h"

#define SDS_LOOP_OFF 0x7F
#define SDS_LOOP_FORWARD 0x00
#define SDS_LOOP_FORWARD_BACKWARD 0x01

class MidiSDSClass {
public:
  Wav wav_file;
  uint8_t deviceID;
  uint8_t packetNumber;
  uint16_t sampleNumber;
  uint8_t sampleFormat;
  uint32_t samplePeriod;
  uint32_t sampleLength;
  uint32_t loopStart;
  uint32_t loopEnd;
  uint8_t loopType;

  uint32_t samplesSoFar;
  bool handShake;

  MidiSDSClass() {
    deviceID = 0x00;
    packetNumber = 0;
    sampleNumber = 0;
    sampleFormat = 16;
    setSampleRate(44100);
    sampleLength = 0;
    loopStart = 0;
    loopEnd = 0;
    loopType = SDS_LOOP_OFF;
    handShake = false;
  }

  void setSampleRate(uint32_t hz) { samplePeriod = (uint32_t)1000000000 / hz; }
  uint8_t waitForAck(uint16_t timeout = 2000);
  bool sendWav(char *filename, uint16_t sample_number, uint8_t loop_type = 0x7F,
               uint32_t loop_start = 0, uint32_t loop_end = 0);
  bool sendSamples();
  void dump_header();
  void sendAckMessage();
  void sendNakMessage();
  void sendCancelMessage();
  void sendWaitMessage();
  void sendEOFMessage();
  void sendGeneralMessage(uint8_t type);
  void sendDumpRequest();
  void sendDumpHeader();
  bool sendData(uint8_t *buf, uint8_t len);
};
extern MidiSDSClass midi_sds;
#endif /* MIDISDS_H__ */
