/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MNM_DATA_ENCODER_H__
#define MNM_DATA_ENCODER_H__

#include "WProgram.h"
#include "ElektronDataEncoder.hh"
#include "DataEncoder.hh"

/**
 * \addtogroup Elektron
 *
 * @{
 *
 * \addtogroup elektron_mnmencoder Monomachine Encoders
 *
 * @{
 *
 * \file
 * Elektron Monomachine encoding and decoding routines
 **/

/**
 * \addtogroup elektron_mnm_data_to_sysex_encoder Monomachine Data to Sysex Encoder
 *
 * @{
 *
 **/

/** Class to encoder normal 8-bit data to 7-bit encoded and compressed sysex data. **/
class MNMDataToSysexEncoder : public ElektronDataToSysexEncoder {
public:
  uint8_t lastByte;
  uint8_t lastCnt;
  bool isFirstByte;
	uint16_t totalCnt;
  
public:
  MNMDataToSysexEncoder(DATA_ENCODER_INIT(uint8_t *_sysex = NULL, uint16_t _sysexLen = 0)) {
    init(DATA_ENCODER_INIT(_sysex, _sysexLen));
  }

	MNMDataToSysexEncoder(MidiUartParent *_uart) {
		init(DATA_ENCODER_INIT(NULL, 0), _uart);
	}

  virtual void init(DATA_ENCODER_INIT(uint8_t *_sysex = NULL, uint16_t _sysexLen = 0),
										MidiUartParent *_uart = NULL);
  DATA_ENCODER_RETURN_TYPE encode7Bit(uint8_t inb);
  virtual DATA_ENCODER_RETURN_TYPE pack8(uint8_t inb);
  DATA_ENCODER_RETURN_TYPE packLastByte();
  virtual uint16_t finish();
};

/** @} **/

/**
 * \addtogroup elektron_mnm_sysex_to_data_encoder Monomachine Sysex to Data Encoder
 *
 * @{
 *
 **/

/** Class to encode 7-bit and compressed sysex data to normal 8-bit data. **/
class MNMSysexToDataEncoder : public ElektronSysexToDataEncoder {
public:
  uint8_t repeat;
	uint16_t totalCnt;
  
  MNMSysexToDataEncoder(DATA_ENCODER_INIT(uint8_t *_data = NULL, uint16_t _maxLen = 0)) {
    init(DATA_ENCODER_INIT(_data, _maxLen));
  }

  virtual void init(DATA_ENCODER_INIT(uint8_t *_data, uint16_t _maxLen));
  virtual DATA_ENCODER_RETURN_TYPE pack8(uint8_t inb);
  DATA_ENCODER_RETURN_TYPE unpack8Bit();
  virtual uint16_t finish();
};


/** @} **/

/**
 * \addtogroup elektron_mnm_sysex_decoder Monomachine Sysex Decoder
 *
 * @{
 *
 **/

/** Class to decode 7-bit encoded and compressed sysex data. **/
class MNMSysexDecoder : public DataDecoder {
public:
  uint8_t cnt7;
  uint8_t bits;
  uint8_t tmpData[7];
	uint16_t cnt;
	uint8_t repeatCount;
	uint8_t repeatByte;
	uint16_t totalCnt;
	
public:
	MNMSysexDecoder(DATA_ENCODER_INIT(uint8_t *_data = NULL, uint16_t _maxLen = 0)) {
		init(DATA_ENCODER_INIT(_data, _maxLen));
	}
	
	virtual void init(DATA_ENCODER_INIT(uint8_t *_data, uint16_t _maxLen));
	virtual DATA_ENCODER_RETURN_TYPE get8(uint8_t *c);
	virtual DATA_ENCODER_RETURN_TYPE getNextByte(uint8_t *c);
};

#endif /* MNM_DATA_ENCODER_H__ */
