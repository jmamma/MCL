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

#include "Elektron.hh"
#include "ElektronDataEncoder.hh"

#define SEND_DELAY_US 1

void ElektronDataToSysexEncoder::init(DATA_ENCODER_INIT(uint8_t *_sysex, uint16_t _sysexLen),
																MidiUartParent *_uart) {
	DataEncoder::init(DATA_ENCODER_INIT(_sysex, _sysexLen));
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

void ElektronDataToSysexEncoder::finishChecksum() {
	uint16_t len = retLen - 5;
	inChecksum = false;
	stop7Bit();
	pack8((checksum >> 7) & 0x7F);
	pack8(checksum & 0x7F);
	pack8((len >> 7) & 0x7F);
	pack8(len & 0x7F);
	pack8(0xF7);
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
            uart->putc(ptr[i]);
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
                uart->putc(data[i]);
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
		if (inChecksum) {
			checksum += inb;
		}
		if (uart != NULL) {
            uart->putc(inb);
		} else {
			*(ptr++) = inb;
		}
		retLen++;
	}
	DATA_ENCODER_TRUE();
}

void ElektronSysexToDataEncoder::init(DATA_ENCODER_INIT(uint8_t *_data, uint16_t _maxLen)) {
	DataEncoder::init(DATA_ENCODER_INIT(_data, _maxLen));
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
		*(ptr++) = tmpData[i];
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

 void ElektronSysexDecoder::init(DATA_ENCODER_INIT(uint8_t *_data, uint16_t _maxLen)) {
	 DataDecoder::init(DATA_ENCODER_INIT(_data, _maxLen));
	 start7Bit();
}

DATA_ENCODER_RETURN_TYPE ElektronSysexDecoder::get8(uint8_t *c) {
	if (in7Bit) {
		if ((cnt7 % 8) == 0) {
			bits = *(ptr++);
			cnt7++;
		}
		bits <<= 1;
		*c = *(ptr++) | (bits & 0x80);
		cnt7++;
	} else {
		*c = *(ptr++);
	}

	DATA_ENCODER_TRUE();
}
