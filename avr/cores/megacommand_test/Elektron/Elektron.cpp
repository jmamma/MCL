/* Copyright (c) 2009 - http://ruinwesen.com/ */

/**
 * \addtogroup Elektron
 *
 * @{
 *
 * \addtogroup elektron_helpers Elektron Helpers
 *
 * @{
 *
 * \file
 * Elektron helper routines and data structures
 **/

#include "Elektron.hh"

uint16_t ElektronHelper::ElektronDataToSysex(uint8_t *data, uint8_t *sysex, uint16_t len) {
  uint16_t retlen = 0;
  uint16_t cnt;
  uint16_t cnt7 = 0;

  sysex[0] = 0;
  for (cnt = 0; cnt < len; cnt++) {
    uint8_t c = data[cnt] & 0x7F;
    uint8_t msb = data[cnt] >> 7;
    sysex[0] |= msb << (6 - cnt7);
    sysex[1 + cnt7] = c;

    if (cnt7++ == 6) {
      sysex += 8;
      retlen += 8;
      sysex[0] = 0;
      cnt7 = 0;
    }
  }
  return retlen + cnt7 + (cnt7 != 0 ? 1 : 0);
}

uint16_t ElektronHelper::ElektronSysexToData(uint8_t *sysex, uint8_t *data, uint16_t len) {
  uint16_t cnt;
  uint16_t cnt2 = 0;
  uint16_t bits = 0;
  for (cnt = 0; cnt < len; cnt++) {
    if ((cnt % 8) == 0) {
      bits = sysex[cnt];
    } else {
      bits <<= 1;
      data[cnt2++] = sysex[cnt] | (bits & 0x80);
    }
  }
  return cnt2;
}

uint16_t ElektronHelper::MNMDataToSysex(uint8_t *data, uint8_t *sysex,
					uint16_t len, uint16_t sysexLen) {
  MNMDataToSysexEncoder encoder(DATA_ENCODER_INIT(sysex, sysexLen));
	encoder.pack(data, len);
  return encoder.finish();
}

uint16_t ElektronHelper::MNMSysexToData(uint8_t *sysex, uint8_t *data,
					uint16_t len, uint16_t maxLen) {
  MNMSysexToDataEncoder encoder(DATA_ENCODER_INIT(data, maxLen));
	encoder.pack(sysex, len);
  return encoder.finish();
}

uint16_t ElektronHelper::to16Bit7(uint8_t b1, uint8_t b2) {
  return (b1 << 7) | b2; 
}

uint16_t ElektronHelper::to16Bit(uint8_t b1, uint8_t b2) {
  return (b1 << 8) | b2; 
}

uint16_t ElektronHelper::to16Bit(uint8_t *b) {
  return (b[0] << 8) | b[1]; 
}


uint32_t ElektronHelper::to32Bit(uint8_t *b) {
  return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | (uint32_t)b[3];
}

void ElektronHelper::from16Bit(uint16_t num, uint8_t *b) {
  b[0] = (num >> 8) & 0xFF;
  b[1] = num & 0xFF;
}

void ElektronHelper::from32Bit(uint32_t num, uint8_t *b) {
  b[0] = (num >> 24) & 0xFF;
  b[1] = (num >> 16) & 0xFF;
  b[2] = (num >> 8) & 0xFF;
  b[3] = (num >> 0) & 0xFF;
}

uint64_t ElektronHelper::to64Bit(uint8_t *b) {
  uint64_t ret = 0;
  ret |= ((uint64_t)(b[0]) << 56);
  ret |= ((uint64_t)(b[1]) << 48);
  ret |= ((uint64_t)(b[2]) << 40);
  ret |= ((uint64_t)(b[3]) << 32);
  ret |= ((uint64_t)(b[4]) << 24);
  ret |= ((uint64_t)(b[5]) << 16);
  ret |= ((uint64_t)(b[6]) << 8);
  ret |= ((uint64_t)(b[7]) << 0);
  return ret;
}

void ElektronHelper::from64Bit(uint64_t num, uint8_t *b) {
  b[0] = (num >> 56) & 0xFF;
  b[1] = (num >> 48) & 0xFF;
  b[2] = (num >> 40) & 0xFF;
  b[3] = (num >> 32) & 0xFF;
  b[4] = (num >> 24) & 0xFF;
  b[5] = (num >> 16) & 0xFF;
  b[6] = (num >> 8) & 0xFF;
  b[7] = (num >> 0) & 0xFF;
}

/* check sysex */
bool ElektronHelper::checkSysexChecksum(uint8_t *data, uint16_t len) {
  uint16_t cksum = 0;
  for (int i = 9 - 6; i < len - 4; i++) {
    cksum += data[i];
  }
  cksum &= 0x3FFF;
	uint16_t realcksum = ElektronHelper::to16Bit7(data[len - 4], data[len - 3]);
  if (cksum != realcksum) {
#ifdef HOST_MIDIDUINO
		printf("wrong checksum, %x should have been %x\n", cksum, realcksum);
#endif
    // wrong checksum
    return false;
  }
	return true;
}

void ElektronHelper::calculateSysexChecksum(uint8_t *data, uint16_t len) {
	data[0] = 0xF0;
  uint16_t checksum = 0;
  for (int i = 9; i < len; i++)
    checksum += data[i];
  data[len] = (uint8_t)((checksum >> 7) & 0x7F);
  data[len + 1] = (uint8_t)(checksum & 0x7F);
  uint16_t length = len + 5 - 7 - 3;
  data[len + 2] = (uint8_t)((length >> 7) &0x7F);
  data[len + 3 ] = (uint8_t)(length & 0x7F);
  data[len + 4] = 0xF7;
}

/* Encoders */

