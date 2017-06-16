/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef DATA_ENCODER_CHECKING_H__
#define DATA_ENCODER_CHECKING_H__

/**
 * \addtogroup CommonTools
 *
 * @{
 *
 * \addtogroup helpers_data_encoder Data encoding classes
 *
 * @{
 *
 * \file
 * Length checking versions of the DataEncoder and the DataDecoder.
 **/

#define DATA_ENCODER_RETURN_TYPE bool
#define DATA_ENCODER_CHECK(condition) { if (!(condition)) return false; }
#define DATA_ENCODER_TRUE() { return true; }
#define DATA_ENCODER_FALSE() { return true; }
#define DATA_ENCODER_INIT(data, length) data, length

#define DATA_ENCODER_CHECKING 1

/**
 * \addtogroup dataencoder Data Encoder
 * Checking version of the data encoder.
 *
 * @{
 */

/** DataEncoder class, packing various values into a data buffer. **/
class DataEncoder {
	/**
	 * \addtogroup dataencoder
	 * @{
	 **/
public:
  uint8_t *data;
  uint8_t *ptr;
  uint16_t maxLen;

  virtual void init(uint8_t *_data, uint16_t _maxLen) {
    data = _data;
    maxLen = _maxLen;
    ptr = data;
  }

	uint16_t getIdx() {
		return ptr - data;
	}

	DATA_ENCODER_RETURN_TYPE pack(uint8_t *inb, uint16_t len) {
		for (uint16_t i = 0; i < len; i++) {
			if (!pack8(inb[i]))
				return false;
		}
		return true;
	}

  virtual DATA_ENCODER_RETURN_TYPE pack8(uint8_t inb) {
    return false;
  }

	virtual DATA_ENCODER_RETURN_TYPE packb(bool inb) {
		if (inb)
			return pack8(1);
		else
			return pack8(0);
  }


	DATA_ENCODER_RETURN_TYPE fill8(uint8_t inb, uint16_t cnt) {
		for (uint16_t i = 0; i < cnt; i++) {
			if (!pack8(inb))
				return false;
		}
		return true;
	}
	
	DATA_ENCODER_RETURN_TYPE pack16(uint16_t inw) {
		return pack8((inw >> 8) & 0xFF) && 
			pack8(inw & 0xFF);
		
	}

	DATA_ENCODER_RETURN_TYPE pack32(uint32_t inw) {
		return pack8((inw >> 24) & 0xFF) &&
			pack8((inw >> 16) & 0xFF) &&
			pack8((inw >> 8) & 0xFF) &&
			pack8(inw & 0xFF);
	}

	DATA_ENCODER_RETURN_TYPE pack32(uint32_t *addr, uint16_t cnt) {
		for (uint16_t i = 0; i < cnt; i++) {
			if (!pack32(addr[i]))
				return false;
		}
		return true;
	}

	DATA_ENCODER_RETURN_TYPE pack32(uint64_t *addr, uint16_t cnt) {
		for (uint16_t i = 0; i < cnt; i++) {
			if (!pack32(addr[i]))
				return false;
		}
		return true;
	}

	DATA_ENCODER_RETURN_TYPE pack32hi(uint64_t inw) {
		return pack32(inw >> 32);
	}


	DATA_ENCODER_RETURN_TYPE pack32hi(uint64_t *addr, uint16_t cnt) {
		for (uint16_t i = 0; i < cnt; i++) {
			if (!pack32hi(addr[i]))
				return false;
		}
		return true;
	}

	DATA_ENCODER_RETURN_TYPE pack64(uint64_t inw) {
		return pack32hi(inw) && pack32(inw & 0xFFFFFFFF);
	}

	DATA_ENCODER_RETURN_TYPE pack64(uint64_t *addr, uint16_t cnt) {
		for (uint16_t i = 0; i < cnt; i++) {
			if (!pack64(addr[i]))
				return false;
		}
		return true;
	}
	
  virtual uint16_t finish() {
    return 0;
  }
#ifdef HOST_MIDIDUINO
  virtual ~DataEncoder() { };
#endif
	/* @} */
};

/** @} **/

/**
 * \addtogroup datadecoder Data Decoder
 * Checking version of the data decoder.
 *
 * @{
 **/

/** DataDecoder class, unpacking various values out of a data buffer. **/
class DataDecoder {
	/**
	 * \addtogroup datadecoder
	 * @{
	 **/
public:
	uint8_t *data;
	uint8_t *ptr;
	uint16_t maxLen;

	DataDecoder() {
	}

	virtual void init(uint8_t *_data, uint16_t _maxLen) {
		data = _data;
		maxLen = _maxLen;
		ptr = data;
	}

	uint16_t getIdx() {
		return ptr - data;
	}

	virtual DATA_ENCODER_RETURN_TYPE getb(bool *b) {
		uint8_t c;
		bool ret = get8(&c);
		if (!ret) {
			*b = c;
		}
		return ret;
	}
	
	virtual DATA_ENCODER_RETURN_TYPE get8(uint8_t *c) {
		return false;
	}

	virtual DATA_ENCODER_RETURN_TYPE skip8() {
		uint8_t b;
		return get8(&b);
	}

	virtual DATA_ENCODER_RETURN_TYPE skip(uint16_t cnt) {
		for (uint16_t i = 0; i < cnt; i++) {
			if (!skip8()) {
				return false;
			}
		}
		return true;
	}
	
	DATA_ENCODER_RETURN_TYPE get16(uint16_t *c) {
		uint8_t b1, b2;
		bool ret = get8(&b1) && get8(&b2);
		if (ret) {
			*c = (((uint16_t)b1) << 8) | b2;
		}
		return ret;
	}
	DATA_ENCODER_RETURN_TYPE get32(uint32_t *c) {
		uint16_t b1, b2;
		bool ret = get16(&b1) && get16(&b2);
		if (ret) {
			*c = (((uint32_t)b1) << 16) | b2;
		}
		return ret;
	}
	DATA_ENCODER_RETURN_TYPE get32(uint32_t *c, uint16_t cnt) {
		for (uint16_t i = 0; i < cnt; i++) {
			if (!get32(&c[i]))
				return false;
		}
		return true;
	}
	DATA_ENCODER_RETURN_TYPE get32(uint64_t *c) {
		uint32_t c2;
		if (!get32(&c2)) {
			return false;
		}
		*c = c2;
		return true;
	}
	DATA_ENCODER_RETURN_TYPE get32(uint64_t *c, uint16_t cnt) {
		for (uint16_t i = 0; i < cnt; i++) {
			if (!get32(&c[i]))
				return false;
		}
		return true;
	}
	DATA_ENCODER_RETURN_TYPE get32hi(uint64_t *c) {
		uint32_t c2;
		if (!get32(&c2)) {
			return false;
		}
		*c |= ((uint64_t)c2) << 32;
		return true;
	}
	DATA_ENCODER_RETURN_TYPE get32hi(uint64_t *c, uint16_t cnt) {
		for (uint16_t i = 0; i < cnt; i++) {
			if (!get32hi(&c[i]))
				return false;
		}
		return true;
	}
	

	DATA_ENCODER_RETURN_TYPE get64(uint64_t *c) {
		uint32_t c1, c2;
		if (!(get32(&c1) && get32(&c2))) {
			return false;
		}
		*c = ((uint64_t)c1 << 32) | c2;
		return true;
	}

	DATA_ENCODER_RETURN_TYPE get64(uint64_t *c, uint16_t cnt) {
		for (uint16_t i = 0; i < cnt; i++) {
			if (!get64(&c[i]))
				return false;
		}
		return true;
	}
	
	uint16_t get(uint8_t *data, uint16_t len) {
		uint16_t i;
		for (i = 0; i < len; i++) {
			if (!get8(data + i))
				break;
		}
		return i;
	}

	uint8_t gget8() {
		uint8_t b;
		get8(&b);
		return b;
	}

	uint16_t gget16() {
		uint16_t a1 = ((uint16_t)gget8()) << 8;
		uint8_t a2 = gget8();
		return a1 | a2;
	}
	
	uint32_t gget32() {
		uint32_t a1 = ((uint32_t)gget16()) << 16;
		uint16_t a2 = gget16();
		return a1 | a2;
	}
	
	uint64_t gget64() {
		uint64_t a1 = ((uint64_t)gget32()) << 32;
		uint32_t a2 = gget32();
		return a1 | a2;
	}
	
#ifdef HOST_MIDIDUINO
  virtual ~DataDecoder() { };
#endif
	/* @} */
};

/* @} @} @} */

#endif /* DATA_ENCODER_CHECKING_H__ */
