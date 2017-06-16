/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef BITARRAY_H__
#define BITARRAY_H__

#include <inttypes.h>
#include "helpers.h"

/**
 * \addtogroup CommonTools
 *
 * @{
 *
 * \addtogroup helpers_cpp_classes C++ classes
 *
 * @{
 *
 * \file
 * Bit array class.
 **/

/**
 * \addtogroup bitarray
 * Class to access a memory region as a huge bit vector.
 *
 * @{
 */

/** Class to access a memory region as a huge bit vector. **/
class BitArray {
	/**
	 *\addtogroup bitarray
	 * @{
	 */
  uint8_t *addr;
  uint8_t offset;
	
public:
	/** Create a bit array starting at _addr, with a bit offset inside the first byte. **/
  BitArray(uint8_t *_addr, uint8_t _offset) {
    addr = _addr;
    offset = _offset;
  };

	/** Get the value of bit idx. **/
  uint8_t getBit(uint16_t idx) {
    idx += offset;
    return IS_BIT_SET(addr[idx >> 3], idx & 7);
  }

	/** Set the bit at idx. **/
  void setBit(uint16_t idx) {
    idx += offset;
    SET_BIT(addr[idx >> 3], idx & 7);
  }

	/** Set or clear the bit at idx. **/
  void setBit(uint16_t idx, uint8_t value) {
    idx += offset;
    if (value) {
      SET_BIT(addr[idx >> 3], idx & 7);
    } else {
      CLEAR_BIT(addr[idx >> 3], idx & 7);
    }
  }

	/** Clear the bit at idx. **/
  void clearBit(uint16_t idx) {
    idx += offset;
    CLEAR_BIT(addr[idx >> 3], idx & 7);
  }

	/** Toggle the bit at idx. **/
  uint8_t toggleBit(uint16_t idx) {
    idx += offset;
    return TOGGLE_BIT(addr[idx >> 3], idx & 7);
  }
	/* @} */
};

/** @} **/

/**
 * \addtogroup bitfield
 * Class representing a bit vector of N bits.
 *
 * @{
 */

/** Class representing a bit vector of N bits. **/
template <int N> class BitField {
	/**
	 *\addtogroup bitfield
	 * @{
	 */
public:
  uint8_t _bits[(N / 8) + 1];

	/** Get the value of bit idx. **/
  uint8_t isBitSet(uint16_t idx) {
    return IS_BIT_SET(_bits[idx >> 3], idx & 7);
  }

	/** Set the bit at idx. **/
  void setBit(uint16_t idx) {
    SET_BIT(_bits[idx >> 3], idx & 7);
  }


	/** Set or clear the bit at idx. **/
  void setBit(uint16_t idx, uint8_t value) {
    if (value) {
      SET_BIT(_bits[idx >> 3], idx & 7);
    } else {
      CLEAR_BIT(_bits[idx >> 3], idx & 7);
    }
  }

	/** Clear the bit at idx. **/
  void clearBit(uint16_t idx) {
    CLEAR_BIT(_bits[idx >> 3], idx & 7);
  }

	/** Toggle the bit at idx. **/
  void toggleBit(uint16_t idx) {
    TOGGLE_BIT(_bits[idx >> 3], idx & 7);
  }

	/* @} */
};

/** @} **/

#endif /* BITARRAY_H__ */
