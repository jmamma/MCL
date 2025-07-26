/* Copyright (c) 2009 - http://ruinwesen.com/ */

#include "Elektron.h"
#include "ElektronDataEncoder.h"
#include "MNMDataEncoder.h"

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


void MNMDataToSysexEncoder::mnm_encoder_init() {
  repeatByte = 0;
  repeatCount = 0;
}

void MNMDataToSysexEncoder::mnm_flush() {
  if (!repeatCount) {
    return;
  } else if (repeatByte < 0x80 && repeatCount == 1) {
    ElektronDataToSysexEncoder::pack8(repeatByte);
  } else {
    ElektronDataToSysexEncoder::pack8(0x80 | repeatCount);
    ElektronDataToSysexEncoder::pack8(repeatByte);
  }
  repeatCount = 0;
}

uint16_t MNMDataToSysexEncoder::finish() {
  mnm_flush();
  return ElektronDataToSysexEncoder::finish();
}

DATA_ENCODER_RETURN_TYPE MNMDataToSysexEncoder::pack8(uint8_t inb) {

  if(!in7Bit) {
    ElektronDataToSysexEncoder::pack8(inb);
    DATA_ENCODER_TRUE();
  }

  if (inb == repeatByte && repeatCount < 0x7F) {
    ++repeatCount;
    DATA_ENCODER_TRUE();
  }

  mnm_flush();

  repeatCount = 1;
  repeatByte = inb;

	DATA_ENCODER_TRUE();
}

void MNMSysexDecoder::mnm_decoder_init() {
	repeatCount = 0;
	repeatByte = 0;
}

DATA_ENCODER_RETURN_TYPE MNMSysexDecoder::get8(uint8_t *c) {

again:
  if (repeatCount > 0) {
    repeatCount--;
    *c = repeatByte;
    DATA_ENCODER_TRUE();
  }

  uint8_t byte;
  DATA_ENCODER_CHECK(ElektronSysexDecoder::get8(&byte));

  if (IS_BIT_SET(byte, 7)) {
    repeatCount = byte & 0x7F;
    DATA_ENCODER_CHECK(ElektronSysexDecoder::get8(&repeatByte));
    goto again;
  } else {
    *c = byte;
    DATA_ENCODER_TRUE();
  }
}
