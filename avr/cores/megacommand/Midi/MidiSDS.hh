#ifndef MIDISDS_H__
#define MIDISDS_H__

class MidiSDSClass {
public:
  uint8_t deviceID;
  uint8_t packetNumber;
  uint16_t sampleNumber;
  uint8_t sampleFormat;
  uint32_t samplePeriod;
  uint32_t sampleLength;
  uint32_t loopStart;
  uint32_t loopEnd;
  uint8_t loopType;

  static const uint8_t LOOP_FORWARD_ONLY = 0x00;
  static const uint8_t LOOP_FORWARD_BACKWARD = 0x01;
  static const uint8_t LOOP_OFF = 0x7F;

  MidiSDSClass() {
    deviceID = 0x00;
    packetNumber = 0;
    sampleNumber = 0;
    sampleFormat = 16;
    setSampleRate(44100);
    sampleLength = 0;
    loopStart = 0;
    loopEnd = 0;
    loopType = LOOP_OFF;
  }

  void setSampleRate(uint32_t hz) {
    samplePeriod = (uint32_t)1000000000 / hz;
  }
  
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

#endif /* MIDISDS_H__ */
