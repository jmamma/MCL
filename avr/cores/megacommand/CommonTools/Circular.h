#ifndef CIRCULAR_H__
#define CIRCULAR_H__

#include <inttypes.h>

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
 * Circular buffer class.
 **/

/**
 * \addtogroup circularbuffer
 * Templated circular buffer of configurable type and size.
 *
 * @{
 */

template <class C, int N>
class CircularBuffer {
	/**
	 * \addtogroup circularbuffer
	 * @{
	 **/
	
public:
  volatile uint8_t start, count;
  C buf[N];

  CircularBuffer() {
    start = count = 0;
  }

	/** Empty the ring buffer. **/
  void clear() {
    start = count = 0;
  }

	/** Add c to the ring buffer. **/
  void put(C c) {
    start = (start + 1) % N;
    buf[start] = c;
    count = MIN(N, count + 1);
  }

	/** Copy the element pointed to by c into the ring buffer. **/
  void putp(C *c) {
    start = (start + 1) % N;
    m_memcpy((void *)&buf[start], c, sizeof(C));
    count = MIN(N, count+1);
  }

	/** Return the size of the ringbuffer. **/
  uint8_t size() {
    return count;
  }

	/** Return the element at index i, no boundary check done. **/
  C get(uint8_t i) {
    return buf[(start + N - i) % N];
  }

	/** Return a pointer to the element at index i, or NULL if outside the boundaries. **/
  C* getp(uint8_t i) {
    if (count > i)
      return &buf[(start + N - i) % N];
    else
      return NULL;
  }

	/** Get a copy of the element at index i into c. **/
  bool getCopy(uint8_t i, C *c) {
    if (count > i) {
      m_memcpy(c, (void *)&buf[(start + N - i) % N], sizeof(C));
      return true;
    } else {
      return false;
    }
  }

	/** Return the last element. **/
  C getLast() {
    return get(count);
  }

	/** Return a pointer to the last element in the ring buffer. */
  C* getLastp() {
    return getp(count);
  }

	/** Copy the last element into c. **/
  bool getLastCopy(C *c) {
    return getCopy(count);
  }

	/* @} */
      
};

/* @} @} @} */

#endif /* CIRCULAR_H__ */
