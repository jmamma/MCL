/* Copyright (c) 2009 - http://ruinwesen.com/ */

#include "Elektron.hh"
#include "ElektronDataEncoder.hh"
#include "MNMDataEncoder.hh"

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

void MNMDataToSysexEncoder::init(DATA_ENCODER_INIT(uint8_t *_sysex, uint16_t _sysexLen), MidiUartParent *_uart)
{
  ElektronDataToSysexEncoder::init(DATA_ENCODER_INIT(_sysex, _sysexLen), _uart);
  lastByte = 0;
  lastCnt = 0;
  isFirstByte = true;
}

DATA_ENCODER_RETURN_TYPE MNMDataToSysexEncoder::pack8(uint8_t inb) {
	//	printf("patck: %x\n", inb);
	totalCnt++;
	if (in7Bit) {
		if (isFirstByte) {
			lastByte = inb;
			lastCnt = 1;
			isFirstByte = false;
			DATA_ENCODER_TRUE();
		} else {
			if (inb == lastByte) {
				lastCnt++;
				if (lastCnt == 127) {
					DATA_ENCODER_CHECK(packLastByte());
				}
			} else {
				DATA_ENCODER_CHECK(packLastByte());
				lastByte = inb;
				lastCnt = 1;
			}
		}
	} else {
		ElektronDataToSysexEncoder::pack8(inb);
	}

	DATA_ENCODER_TRUE();
}

DATA_ENCODER_RETURN_TYPE MNMDataToSysexEncoder::packLastByte() {
  if (lastCnt > 0) {
    if ((lastCnt == 1) && !(lastByte & 0x80)) {
			DATA_ENCODER_CHECK(ElektronDataToSysexEncoder::pack8(lastByte));
      lastCnt = 0;
    } else {
			DATA_ENCODER_CHECK(ElektronDataToSysexEncoder::pack8(0x80 | lastCnt));
      DATA_ENCODER_CHECK(ElektronDataToSysexEncoder::pack8(lastByte));
      lastCnt = 0;
    }
	}

	DATA_ENCODER_TRUE();
}

uint16_t MNMDataToSysexEncoder::finish() {
#ifdef DATA_ENCODER_CHECKING
  if (!packLastByte())
    return 0;
  else
		return ElektronDataToSysexEncoder::finish();
#else
	packLastByte();
	ElektronDataToSysexEncoder::finish();
#endif
}

void MNMSysexToDataEncoder::init(DATA_ENCODER_INIT(uint8_t *_data, uint16_t _maxLen)) {
  ElektronSysexToDataEncoder::init(DATA_ENCODER_INIT(_data, _maxLen));
  repeat = 0;
	totalCnt = 0;
}

DATA_ENCODER_RETURN_TYPE MNMSysexToDataEncoder::pack8(uint8_t inb) {
  //  printf("pack: %x\n", inb);
	totalCnt++;
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

DATA_ENCODER_RETURN_TYPE MNMSysexToDataEncoder::unpack8Bit() {
  for (uint8_t i = 0; i < cnt7; i++) {
    //    printf("tmpdata[%d]: %x\n", i, tmpData[i]);
    if (repeat == 0) {
      if (tmpData[i] & 0x80) {
				repeat = tmpData[i] & 0x7F;
      } else {
#ifdef DATA_ENCODER_CHECKING
				DATA_ENCODER_CHECK(retLen <= maxLen);
#endif
				*(ptr++) = tmpData[i];
				retLen++;
      }
    } else {
      for (uint8_t j = 0; j < repeat; j++) {
#ifdef DATA_ENCODER_CHECKING
				DATA_ENCODER_CHECK(retLen <= maxLen);
#endif
				*(ptr++) = tmpData[i];
				retLen++;
      }
      repeat = 0;
    }
  }
  cnt7 = 0;

	DATA_ENCODER_TRUE();
}

uint16_t MNMSysexToDataEncoder::finish() {
#ifdef DATA_ENCODER_CHECKING
  //  printf("cnt7: %d\n", cnt7);
  if (!unpack8Bit()) {
    return 0;
  }
#else
	unpack8Bit();
#endif
  return retLen;
	
}

void MNMSysexDecoder::init(DATA_ENCODER_INIT(uint8_t *_data, uint16_t _maxLen)) {
	DataDecoder::init(DATA_ENCODER_INIT(_data, _maxLen));
	cnt7 = 0;
	cnt = 0;
	repeatCount = 0;
	repeatByte = 0;
	totalCnt = 0;
}

DATA_ENCODER_RETURN_TYPE MNMSysexDecoder::getNextByte(uint8_t *c) {
	if ((cnt % 8) == 0) {
		bits = *(ptr++);
		cnt++;
	}
	bits <<= 1;
	*c = *(ptr++) | (bits & 0x80);
	cnt++;

	DATA_ENCODER_TRUE();
}

DATA_ENCODER_RETURN_TYPE MNMSysexDecoder::get8(uint8_t *c) {
	uint8_t byte;

	totalCnt++;

 again:
	if (repeatCount > 0) {
		repeatCount--;
		*c = repeatByte;
		DATA_ENCODER_TRUE();
	}

	DATA_ENCODER_CHECK(getNextByte(&byte));

	if (IS_BIT_SET(byte, 7)) {
		repeatCount = byte & 0x7F;
		DATA_ENCODER_CHECK(getNextByte(&repeatByte));
		goto again;
	} else {
		*c = byte;
		DATA_ENCODER_TRUE();
	}
}
