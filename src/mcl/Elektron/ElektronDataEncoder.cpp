/* Copyright (c) 2009 - http://ruinwesen.com/ */

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

#include "Elektron.h"
#include "ElektronDataEncoder.h"
#include "Midi.h"
#include "MidiUart.h"
void ElektronDataToSysexEncoder::init(uint8_t *_sysex,
                                      MidiUartClass *_uart) {
  DataEncoder::init(_sysex);
  uart = _uart;
  if (uart != NULL) {
    data = ptr = buf;
  }

  inChecksum = false;
  checksum = 0;
  retLen = 0;
#if !defined(__AVR__)
  inRLE = false;
  rleCount = 0;
  rleByte = 0;
#endif
  start7Bit();
}

void ElektronDataToSysexEncoder::start7Bit() {
  in7Bit = true;
  cnt7 = 0;
}

void ElektronDataToSysexEncoder::stop7Bit() {
  finish();
  in7Bit = false;
  cnt7 = 0;
}

void ElektronDataToSysexEncoder::reset() {
  finish();
  start7Bit();
}

void ElektronDataToSysexEncoder::startChecksum() {
  checksum = 0;
  inChecksum = true;
}

void ElektronDataToSysexEncoder::uart_send(uint8_t c) {
  uart->m_putc(c);
}
void ElektronDataToSysexEncoder::finishChecksum() {
  uint16_t len = retLen - 5;
  inChecksum = false;
  stop7Bit();
  pack8((checksum >> 7) & 0x7F);
  pack8(checksum & 0x7F);
  pack8((len >> 7) & 0x7F);
  pack8(len & 0x7F);
  if (uart != NULL) {
    uart_send(0xF7);
  } else {
    *(ptr++) = 0xF7;
  }
}

void ElektronDataToSysexEncoder::begin() {
  if (uart != NULL) {
    uart_send(0xF0);
  } else {
    *(ptr++) = 0xF0;
  }

  retLen++;
}

uint16_t ElektronDataToSysexEncoder::finish() {
  uint8_t inc = ((cnt7 > 0) ? (cnt7 + 1) : 0);
  cnt7 = 0;
  if (inChecksum) {
    for (uint8_t i = 0; i < inc; i++) {
      checksum += ptr[i];
    }
  }
  if (uart != NULL) {
    for (uint8_t i = 0; i < inc; i++) {
      uart_send(ptr[i]);
    }
    ptr = data;
  } else {
    ptr += inc;
  }
  retLen += inc;
  return retLen;
}

DATA_ENCODER_RETURN_TYPE ElektronDataToSysexEncoder::encode7Bit(uint8_t inb) {
  uint8_t msb = inb >> 7;
  uint8_t c = inb & 0x7F;

#ifdef DATA_ENCODER_CHECKING
  DATA_ENCODER_CHECK((ptr + cnt7 + 1) < (data + maxLen));
#endif

  if (cnt7 == 0) {
    ptr[0] = 0;
  }
  ptr[0] |= msb << (6 - cnt7);
  ptr[cnt7 + 1] = c;
  if (++cnt7 == 7) {
    retLen += 8;
    if (inChecksum) {
      for (uint8_t i = 0; i < 8; i++) {
        checksum += ptr[i];
      }
    }
    if (uart != NULL) {
      for (uint8_t i = 0; i < 8; i++) {
        uart_send(data[i]);
      }
      ptr = data;
    } else {
      ptr += 8;
    }
    cnt7 = 0;
  }
  DATA_ENCODER_TRUE();
}

#if !defined(__AVR__)
void ElektronDataToSysexEncoder::startRLE() {
  inRLE = true;
  rleCount = 0;
  rleByte = 0;
}

void ElektronDataToSysexEncoder::stopRLE() {
  rleFlush();
  inRLE = false;
}

void ElektronDataToSysexEncoder::rleFlush() {
  if (!rleCount) return;
  if (rleByte < 0x80 && rleCount == 1) {
    encode7Bit(rleByte);
  } else {
    encode7Bit(0x80 | rleCount);
    encode7Bit(rleByte);
  }
  rleCount = 0;
}
#endif

DATA_ENCODER_RETURN_TYPE ElektronDataToSysexEncoder::pack8(uint8_t inb) {
#if !defined(__AVR__)
  if (inRLE) {
    if (inb == rleByte && rleCount < 0x7F) {
      rleCount++;
      DATA_ENCODER_TRUE();
    }
    rleFlush();
    rleCount = 1;
    rleByte = inb;
    DATA_ENCODER_TRUE();
  }
#endif
  if (in7Bit) {
    DATA_ENCODER_CHECK(encode7Bit(inb));
  } else {
#ifdef DATA_ENCODER_CHECKING
    DATA_ENCODER_CHECK((ptr + 1) < (data + maxLen));
#endif

    inb &= 0x7F;
    if (inChecksum) {
      checksum += inb;
    }
    if (uart != NULL) {
      uart_send(inb);
    } else {
      *(ptr++) = inb;
    }
    retLen++;
  }
  DATA_ENCODER_TRUE();
}

void ElektronSysexToDataEncoder::init(uint8_t *_data) {
  DataEncoder::init(_data);
  cnt7 = 0;
  cnt = 0;
  retLen = 0;
}
void ElektronSysexToDataEncoder::init(MidiClass *_midi, uint16_t _offset) {
  DataEncoder::init(_midi, _offset);
  cnt7 = 0;
  cnt = 0;
  retLen = 0;
}

DATA_ENCODER_RETURN_TYPE ElektronSysexToDataEncoder::pack8(uint8_t inb) {
  if ((cnt % 8) == 0) {
    bits = inb;
  } else {
    bits <<= 1;
    tmpData[cnt7++] = inb | (bits & 0x80);
  }
  cnt++;

  if (cnt7 == 7) {
    DATA_ENCODER_CHECK(unpack8Bit());
  }
  DATA_ENCODER_TRUE();
}

DATA_ENCODER_RETURN_TYPE ElektronSysexToDataEncoder::unpack8Bit() {
  for (uint8_t i = 0; i < cnt7; i++) {
    if (data) {
      *(ptr++) = tmpData[i];
    } else {
      DEBUG_PRINTLN("ElektronSysexToDataEncoder");
      midi->midiSysex->putByte(n++, tmpData[i]);
    }
    retLen++;
  }
  cnt7 = 0;
  DATA_ENCODER_TRUE();
}

uint16_t ElektronSysexToDataEncoder::finish() {
#ifdef DATA_ENCODER_CHECKING
  if (!unpack8Bit())
    return 0;
  else
    return retLen;
#else
  unpack8Bit();
  return retLen;
#endif
}

void ElektronSysexDecoder::init(uint8_t *_data) {
  DataDecoder::init(_data);
#if !defined(__AVR__)
  inRLE = false;
  rleCount = 0;
  rleByte = 0;
#endif
  start7Bit();
}

void ElektronSysexDecoder::init(MidiClass *_midi, uint16_t _offset) {
  DataDecoder::init(_midi, _offset);
#if !defined(__AVR__)
  inRLE = false;
  rleCount = 0;
  rleByte = 0;
#endif
  start7Bit();
}

// Add this inline helper method
uint8_t ElektronSysexDecoder::readByte() {
  return data ? *(ptr++) : midi->midiSysex->getByte(n++);
}

#if !defined(__AVR__)
void ElektronSysexDecoder::startRLE() {
  inRLE = true;
  rleCount = 0;
  rleByte = 0;
}

void ElektronSysexDecoder::stopRLE() {
  inRLE = false;
  rleCount = 0;
}

void ElektronSysexDecoder::getRaw8(uint8_t *c) {
  if (in7Bit) {
    if ((cnt7 % 8) == 0) {
      bits = readByte();
      cnt7++;
    }
    bits <<= 1;
    *c = readByte() | (bits & 0x80);
    cnt7++;
  } else {
    *c = readByte();
  }
}
#endif

DATA_ENCODER_RETURN_TYPE ElektronSysexDecoder::get8(uint8_t *c) {
#if !defined(__AVR__)
  if (inRLE) {
    if (rleCount > 0) {
      rleCount--;
      *c = rleByte;
      DATA_ENCODER_TRUE();
    }
    uint8_t b;
    getRaw8(&b);
    if (b & 0x80) {
      uint8_t count = b & 0x7F;
      getRaw8(&rleByte);
      if (count > 1) {
        rleCount = count - 1;
      }
      *c = rleByte;
    } else {
      *c = b;
    }
    DATA_ENCODER_TRUE();
  }
#endif
  if (in7Bit) {
    if ((cnt7 % 8) == 0) {
      bits = readByte();
      cnt7++;
    }
    bits <<= 1;
    *c = readByte() | (bits & 0x80);
    cnt7++;
  } else {
    *c = readByte();
  }
  DATA_ENCODER_TRUE();
}


