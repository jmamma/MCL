/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef ELEKTRON_DATA_ENCODER_H__
#define ELEKTRON_DATA_ENCODER_H__

#include "DataEncoder.h"
#include "Midi.h"
#include "WProgram.h"

#define THROTTLE_US 140

/**
 * \addtogroup Elektron
 *
 * @{
 *
 * \addtogroup elektron_encoding Elektron Sysex Encoding and Decoding
 *
 * @{
 *
 * \file
 * Elektron data encoders and decoders
 **/

/**
 * \addtogroup elektron_datatosysexencoder Elektron Data To Sysex Encoder
 *
 * @{
 **/

/** Class to convert normal data into elektron sysex. **/
class ElektronDataToSysexEncoder : public DataEncoder {
protected:
  uint16_t retLen;
  uint16_t cnt7;
  bool in7Bit;
  MidiUartClass *uart;
  uint8_t buf[8];
  uint16_t checksum;
  bool inChecksum;
public:

  ElektronDataToSysexEncoder(uint8_t *_sysex = nullptr) {
    init(_sysex);
  }

  ElektronDataToSysexEncoder(MidiUartClass *_uart) {
    init(0, _uart);
  }

  void uart_send(uint8_t c);
  /** Start the conversion of 8-bit data into 7-bit data. **/
  void start7Bit();
  /** Finish the conversion of 8-bit data into 7-bit data. This will flush
   * remaining 7-bit bytes. **/
  void stop7Bit();
  /** Finish the current conversion and flush remaining data, and start 7-bit
   * conversion. **/
  void reset();

  /** send sysex start message **/
  void begin();

  /** Start adding outgoing bytes to the checksum. **/

  void startChecksum();

  /** Finish the calculation of the checksum and store the length and
   * checksum of the encoded message into the message.
   **/
  void finishChecksum();

  /** Finish the current conversion and flush remaining data. **/
  virtual uint16_t finish();

  virtual void init(uint8_t *_sysex = nullptr, MidiUartClass *_uart = nullptr);
  DATA_ENCODER_RETURN_TYPE encode7Bit(uint8_t inb);

  virtual DATA_ENCODER_RETURN_TYPE pack8(uint8_t inb);
};

/** @} **/

/**
 * \addtogroup elektron_sysextodataencoder Elektron Sysex to Data Encoder
 *
 * @{
 **/

/** Class to encode sysex data to normal data. **/
class ElektronSysexToDataEncoder : public DataEncoder {
protected:
  uint16_t retLen;
  uint8_t cnt7;
  uint8_t bits;
  uint8_t tmpData[7];
  uint16_t cnt;

public:
  ElektronSysexToDataEncoder(uint8_t *_sysex = NULL, uint16_t _sysexLen = 0) {
    init(_sysex);
  }

  ElektronSysexToDataEncoder(MidiClass *_midi, uint16_t _offset) {
    init(_midi, _offset);
  }


  virtual void init(uint8_t *_sysex);
  virtual void init(MidiClass *_midi, uint16_t _offset);

  virtual DATA_ENCODER_RETURN_TYPE pack8(uint8_t inb);
  DATA_ENCODER_RETURN_TYPE unpack8Bit();
  virtual uint16_t finish();
};

/** @} **/

/**
 * \addtogroup elektron_sysexdecoder Elektron Sysex Decoder
 *
 * @{
 **/

/** Class to decode sysex data to normal data. **/
class ElektronSysexDecoder : public DataDecoder {
  uint8_t cnt7;
  uint8_t bits;
  uint8_t tmpData[7];
  bool in7Bit;

public:
  ElektronSysexDecoder(uint8_t *_data = nullptr) {
    init(_data);
  }

  ElektronSysexDecoder(MidiClass *_midi, uint16_t _offset) {
    init(_midi, _offset);
  }

  /** Start the decoding of 7-bit data. **/
  void start7Bit() {
    in7Bit = true;
    cnt7 = 0;
  }

  /** Stop the decoding of 7-bit data. **/
  void stop7Bit() { in7Bit = false; }

  virtual void init(MidiClass *_midi, uint16_t _offset);
  virtual void init(uint8_t *_data);
  virtual DATA_ENCODER_RETURN_TYPE get8(uint8_t *c);
};

#endif /* ELEKTRON_DATA_ENCODER_H__ */
