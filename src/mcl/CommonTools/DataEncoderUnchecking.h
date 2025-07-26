/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef DATA_ENCODER_UNCHECKING_H__
#define DATA_ENCODER_UNCHECKING_H__

#include "Midi.h"

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

#define DATA_ENCODER_RETURN_TYPE void
#define DATA_ENCODER_CHECK(condition) { condition; }
#define DATA_ENCODER_TRUE()  return
#define DATA_ENCODER_FALSE() return

//Required for Macro argument overloading
//#define GET_MACRO(_1,_2,_3,NAME,...) NAME
//#define DATA_ENCODER_INIT(...) GET_MACRO(__VA_ARGS__, DATA_ENCODER_INIT3, DATA_ENCODER_INIT2)(__VA_ARGS__)
//#define DATA_ENCODER_INIT2(data, length) data
//#define DATA_ENCODER_INIT3(midi, offset, length) midi, offset

#define DATA_ENCODER_UNCHECKING 1

/**
 * \addtogroup dataencoder Data Encoder
 * Checking version of the data encoder.
 *
 * @{
 **/

/** DataEncoder class, packing various values into a data buffer. **/
class DataEncoder {
	/**
	 * \addtogroup dataencoder
	 * @{
	 **/
public:
  uint8_t *data;
  uint8_t *ptr;

  uint16_t n;
  uint16_t offset;

  MidiClass *midi;

  virtual void init(uint8_t *_data) {
    data = _data;
    ptr = data;
  }

  virtual void init(MidiClass *_midi, uint16_t _offset) {
        offset = _offset;
        n = offset;
        midi = _midi;
        data = ptr = NULL;
 }
  
  template <typename T>
  DATA_ENCODER_RETURN_TYPE pack(const T &in) {
		uint8_t *inb = (uint8_t *)&in;
		for (uint16_t i = 0; i < sizeof(T); i++)
		{
			pack8(inb[i]);
		}
	}
  
	DATA_ENCODER_RETURN_TYPE pack(uint8_t *inb, uint16_t len) {
		for (uint16_t i = 0; i < len; i++)
		{
			pack8(inb[i]);
		}
	}

	virtual DATA_ENCODER_RETURN_TYPE packb(bool inb) {
		pack8((uint8_t)inb);
	}

	uint16_t getIdx() {
		return ptr - data;
	}
		
  // An actual encoder implement this to provide encoding capabilities.
  virtual DATA_ENCODER_RETURN_TYPE pack8(uint8_t inb) = 0;

	DATA_ENCODER_RETURN_TYPE fill8(uint8_t inb, uint16_t cnt) {
		for (uint16_t i = 0; i < cnt; i++) {
			pack8(inb);
		}
	}
	
	DATA_ENCODER_RETURN_TYPE pack16(uint16_t inw) {
		pack8((inw >> 8) & 0xFF);
		pack8(inw & 0xFF);
	}

	DATA_ENCODER_RETURN_TYPE pack32(uint32_t inw) {
		pack8((inw >> 24) & 0xFF);
		pack8((inw >> 16) & 0xFF);
		pack8((inw >> 8) & 0xFF);
		pack8(inw & 0xFF);
	}

	DATA_ENCODER_RETURN_TYPE pack32(uint32_t *addr, uint16_t cnt) {
		for (uint16_t i = 0; i < cnt; i++) {
			pack32(addr[i]);
		}
	}

	DATA_ENCODER_RETURN_TYPE pack32(uint64_t *addr, uint16_t cnt) {
		for (uint16_t i = 0; i < cnt; i++) {
			pack32(addr[i]);
		}
	}

	DATA_ENCODER_RETURN_TYPE pack32hi(uint64_t inw) {
		pack32(inw >> 32);
	}

	DATA_ENCODER_RETURN_TYPE pack32hi(uint64_t *addr, uint16_t cnt) {
		for (uint16_t i = 0; i < cnt; i++) {
			pack32hi(addr[i]);
		}
	}
	
	DATA_ENCODER_RETURN_TYPE pack64(uint64_t inw) {
		pack32hi(inw);
		pack32(inw & 0xFFFFFFFF);
	}

	DATA_ENCODER_RETURN_TYPE pack64(uint64_t *addr, uint16_t cnt) {
		for (uint16_t i = 0; i < cnt; i++) {
			pack64(addr[i]);
		}
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

  uint16_t n;
  uint16_t offset;

  MidiClass *midi;

  DataDecoder() {
  }

  virtual void init(uint8_t *_data) {
    data = _data;
    ptr = data;
  }

  virtual void init(MidiClass *_midi, uint16_t _offset) {
    offset = _offset;
    n = offset;
    midi = _midi;
    data = ptr = NULL;
  }

  // An actual decoder overrides this to provide decoding capabilities.
	virtual DATA_ENCODER_RETURN_TYPE get8(uint8_t *c) = 0;

	uint16_t getIdx() {
		return ptr - data;
	}

	virtual DATA_ENCODER_RETURN_TYPE getb(bool *b) {
		uint8_t c;
		get8(&c);
		*b = c;
	}
		
	virtual DATA_ENCODER_RETURN_TYPE skip8() {
		uint8_t b;
		get8(&b);
	}

	virtual DATA_ENCODER_RETURN_TYPE skip(uint16_t cnt) {
		for (uint16_t i = 0; i < cnt; i++) {
			skip8();
		}
	}	

	DATA_ENCODER_RETURN_TYPE get16(uint16_t *c) {
		uint8_t b1, b2;
		get8(&b1);
		get8(&b2);
		*c = (((uint16_t)b1) << 8) | b2;
	}

	DATA_ENCODER_RETURN_TYPE get32(uint32_t *c) {
		uint16_t b1, b2;
		get16(&b1);
		get16(&b2);
		*c = (((uint32_t)b1) << 16) | b2;
	}

	DATA_ENCODER_RETURN_TYPE get32(uint32_t *c, uint16_t cnt) {
		for (uint16_t i = 0; i < cnt; i++) {
			get32(&c[i]);
		}
	}

	DATA_ENCODER_RETURN_TYPE get32(uint64_t *c) {
		uint32_t c2;
		get32(&c2);
		*c = c2;
	}

	DATA_ENCODER_RETURN_TYPE get32(uint64_t *c, uint16_t cnt) {
		for (uint16_t i = 0; i < cnt; i++) {
			get32(&c[i]);
		}
	}

	DATA_ENCODER_RETURN_TYPE get32hi(uint64_t *c) {
		uint32_t c2;
		get32(&c2);
		*c |= ((uint64_t)c2) << 32;
	}

	DATA_ENCODER_RETURN_TYPE get32hi(uint64_t *c, uint16_t cnt) {
		for (uint16_t i = 0; i < cnt; i++) {
			get32hi(&c[i]);
		}
	}

	DATA_ENCODER_RETURN_TYPE get64(uint64_t *c) {
		uint32_t c1, c2;
		get32(&c1);
		get32(&c2);
		*c = ((uint64_t)c1 << 32) | c2;
	}

	DATA_ENCODER_RETURN_TYPE get64(uint64_t *c, uint16_t cnt) {
		for (uint16_t i = 0; i < cnt; i++) {
			get64(&c[i]);
		}
	}
  
	template<typename T>
	uint16_t get(const T& data) {
		uint16_t i;
		uint8_t* pdata = (uint8_t *) &data;
		for (i = 0; i < sizeof(data); i++) {
			get8(pdata + i);
		}
		return i;
	}

	uint16_t get(uint8_t *data, uint16_t len) {
		uint16_t i;
		for (i = 0; i < len; i++) {
			get8(data + i);
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

#endif /* DATA_ENCODER_UNCHECKING_H__ */
