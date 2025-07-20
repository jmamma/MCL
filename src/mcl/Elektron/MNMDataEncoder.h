/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MNM_DATA_ENCODER_H__
#define MNM_DATA_ENCODER_H__

#include "DataEncoder.h"
#include "ElektronDataEncoder.h"
#include "WProgram.h"

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
 * \addtogroup elektron_mnm_data_to_sysex_encoder Monomachine Data to Sysex
 *Encoder
 *
 * @{
 *
 **/

/** Class to encoder normal 8-bit data to 7-bit encoded and compressed sysex
 * data. **/
class MNMDataToSysexEncoder : public ElektronDataToSysexEncoder {
private:
  uint8_t repeatByte;
  uint8_t repeatCount;
public:
  MNMDataToSysexEncoder(uint8_t *_sysex = nullptr)
  : ElektronDataToSysexEncoder(_sysex) {
    mnm_encoder_init();
  }

  MNMDataToSysexEncoder(MidiUartClass *_uart) 
  : ElektronDataToSysexEncoder(_uart) {
    mnm_encoder_init();
  }

  void mnm_encoder_init();
  void mnm_flush();
  virtual uint16_t finish();
  virtual DATA_ENCODER_RETURN_TYPE pack8(uint8_t inb);
};

/** @} **/

/**
 * \addtogroup elektron_mnm_sysex_decoder Monomachine Sysex Decoder
 *
 * @{
 *
 **/

/** Class to decode 7-bit encoded and compressed sysex data. **/
class MNMSysexDecoder : public ElektronSysexDecoder {
public:
  uint8_t repeatCount;
  uint8_t repeatByte;

public:
  MNMSysexDecoder(uint8_t *_data = nullptr)
    : ElektronSysexDecoder(_data) { 
      mnm_decoder_init();
  }
  MNMSysexDecoder(MidiClass *_midi = nullptr, uint16_t _offset = 0)
    : ElektronSysexDecoder(_midi, _offset) {
      mnm_decoder_init();
  }

  void mnm_decoder_init();
  virtual DATA_ENCODER_RETURN_TYPE get8(uint8_t *c);
};

#endif /* MNM_DATA_ENCODER_H__ */
