/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef ELEKTRON_DATA_ENCODER_H__
#define ELEKTRON_DATA_ENCODER_H__

#include "WProgram.h"
#include "Midi.h"
#include "DataEncoder.hh"

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
	MidiUartParent *uart;
	uint8_t buf[8];
	uint16_t checksum;
	bool inChecksum;

public:
	ElektronDataToSysexEncoder(DATA_ENCODER_INIT(uint8_t *_sysex = NULL, uint16_t _sysexLen = 0)) {
		init(DATA_ENCODER_INIT(_sysex, _sysexLen));
	}

	ElektronDataToSysexEncoder(MidiUartParent *_uart) {
		init(DATA_ENCODER_INIT(NULL, 0), _uart);
	}

    /** Start the conversion of 8-bit data into 7-bit data. **/
	void start7Bit();
	/** Finish the conversion of 8-bit data into 7-bit data. This will flush remaining 7-bit bytes. **/
	void stop7Bit();

	/** Finish the current conversion and flush remaining data, and start 7-bit conversion. **/
	void reset();

	/** Start adding outgoing bytes to the checksum. **/
	void startChecksum();

	/** Finish the calculation of the checksum and store the length and
	 * checksum of the encoded message into the message.
	 **/
	void finishChecksum();

	/** Finish the current conversion and flush remaining data. **/
	virtual uint16_t finish();
	
	virtual void init(DATA_ENCODER_INIT(uint8_t *_sysex = NULL, uint16_t _sysexLen = 0),
										MidiUartParent *_uart = NULL);
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
		init(DATA_ENCODER_INIT(_sysex, _sysexLen));
	}

	virtual void init(DATA_ENCODER_INIT(uint8_t *_sysex, uint16_t _sysexLen));
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
	ElektronSysexDecoder(DATA_ENCODER_INIT(uint8_t *_data = NULL, uint16_t _maxLen = 0)) {
		init(DATA_ENCODER_INIT(_data, _maxLen));
	}

	/** Start the decoding of 7-bit data. **/
	void start7Bit() {
		in7Bit = true;
		cnt7 = 0;
	}

	/** Stop the decoding of 7-bit data. **/
	void stop7Bit() {
		in7Bit = false;
	}
	
	virtual void init(DATA_ENCODER_INIT(uint8_t *_data, uint16_t _maxLen));
	virtual DATA_ENCODER_RETURN_TYPE get8(uint8_t *c);
};

#endif /* ELEKTRON_DATA_ENCODER_H__ */
