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

void ElektronDataToSysexEncoder::init(uint8_t *_sysex,
                                      MidiUartParent *_uart) {
  DataEncoder::init(_sysex);
  uart = _uart;
  if (uart != NULL) {
    data = ptr = buf;
  }

  inChecksum = false;
  checksum = 0;
  retLen = 0;
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
  if (throttle) {
    /* bool do_throttle = false;
    uint8_t v = 0;
    if (MidiClock.mod6_counter == 0) {
      do_throttle = true;
    }
    if (MidiClock.mod12_counter == throttle_mod12) {
      do_throttle = true;
    }
    if (do_throttle) { */
    delayMicroseconds(THROTTLE_US);
    uart->m_putc_immediate(c);
  } else {
    uart->m_putc(c);
  }
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

DATA_ENCODER_RETURN_TYPE ElektronDataToSysexEncoder::pack8(uint8_t inb) {
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
      midi->midiSysex.putByte(n++, tmpData[i]);
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
  start7Bit();
}

void ElektronSysexDecoder::init(MidiClass *_midi, uint16_t _offset) {
  DataDecoder::init(_midi, _offset);
  start7Bit();
}

DATA_ENCODER_RETURN_TYPE ElektronSysexDecoder::get8(uint8_t *c) {

  if (in7Bit) {
    if ((cnt7 % 8) == 0) {
      if (data) {
        bits = *(ptr++);
      } else {
        bits = midi->midiSysex.getByte(n++);
      }
      cnt7++;
    }
    bits <<= 1;
    if (data) {
      *c = *(ptr++) | (bits & 0x80);
    } else {
      *c = midi->midiSysex.getByte(n++) | (bits & 0x80);
    }
    cnt7++;
  } else {
    if (data) {
      *c = *(ptr++);
    } else {
      *c = midi->midiSysex.getByte(n++);
    }
  }

  DATA_ENCODER_TRUE();
}
