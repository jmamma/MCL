/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef RINGBUFFER_H__
#define RINGBUFFER_H__

#include "WProgram.h"
#include <inttypes.h>

/**
 * \addtogroup CommonTools
 *
 * @{
 *
 * \file
 * Ring buffer classes.
 **/

/**
 * \addtogroup helpers_cringbuffer Ring Buffer class
 *
 * @{
 **/

/**
 * Class representing a ring buffer of N elements of type C, with the
 * index variable of type uint8_t. If your ringbuffer has more than
 * 255 elements, use T = uint16_t.
 **/
template <class C, int N, class T = uint8_t> class CRingBuffer {
  /**
   * \addtogroup helpers_cringbuffer
   * @{
   **/

public:
  volatile T rd, wr;
  volatile C buf[N];
  volatile T len = N;
  volatile C *ptr = NULL;

  volatile uint8_t overflow;

  CRingBuffer(volatile uint8_t *ptr = NULL);
  /** Add a new element c to the ring buffer. **/
  bool put(C c) volatile;
  /** Copy a new element pointed to by c to the ring buffer. **/
  bool putp(C *c) volatile;
  /** Return the next element in the ring buffer. **/
  C get() volatile;
  /** Copy the next element into dst. **/
  bool getp(C *dst) volatile;
  /** Get the next element without removing it from the ring buffer. **/
  C peek() volatile;
  /** Returns true if the ring buffer is empty. **/
  inline bool isEmpty() volatile;
  /** Returns true if the ring buffer is full. **/
  inline bool isFull() volatile;
  /** Returns the number of elements in the ring buffer. **/
  T size() volatile;

  /* @} */
};

template <int N, class T = uint8_t>
class RingBuffer : public CRingBuffer<uint8_t, N, T> {
public:
  RingBuffer(){};
};

template <class C, int N, class T>
CRingBuffer<C, N, T>::CRingBuffer(volatile uint8_t *_ptr) {
  ptr = reinterpret_cast<volatile C *>(_ptr);
  rd = 0;
  wr = 0;
  overflow = 0;
}

template <class C, int N, class T>
bool CRingBuffer<C, N, T>::put(C c) volatile {
  USE_LOCK();
  SET_LOCK();
  if (isFull()) {
    overflow++;
    return false;
  }
  if (ptr == NULL) {
    buf[wr] = c;
  } else {
    put_bank1(ptr + wr, c);
  }
  wr++;
  if (wr == len) {
    wr = 0;
  }
  CLEAR_LOCK();
  return true;
}

template <class C, int N, class T> T CRingBuffer<C, N, T>::size() volatile {
  if (wr >= rd) {
    return wr - rd;
  } else {
    return 256 << (sizeof(T) - 1) - rd + wr;
  }
}

template <class C, int N, class T>
bool CRingBuffer<C, N, T>::putp(C *c) volatile {
  if (isFull()) {
    overflow++;
    return false;
  }
  USE_LOCK();
  SET_LOCK();
  if (ptr == NULL) {
    memcpy((void *)&buf[wr], (void *)c, sizeof(*c));
  } else {
    memcpy_bank1((void *)&(ptr)[wr], (void *)c, sizeof(*c));
  }
  wr++;
  if (wr == len) {
    wr = 0;
  }
  CLEAR_LOCK();
  return true;
}

template <class C, int N, class T> C CRingBuffer<C, N, T>::get() volatile {
  if (isEmpty())
    return 0;
  USE_LOCK();
  SET_LOCK();
  C ret;
  if (ptr == NULL) {
    ret = buf[rd];
  } else {
    ret = get_bank1(ptr + rd);
  }
  rd++;
  if (rd == len) {
    rd = 0;
  }
  CLEAR_LOCK();
  return ret;
}

template <class C, int N, class T>
bool CRingBuffer<C, N, T>::getp(C *dst) volatile {
  if (isEmpty())
    return false;
  USE_LOCK();
  SET_LOCK();
  if (ptr == NULL) {
    memcpy(dst, (void *)&buf[rd], sizeof(C));
  } else {
    memcpy_bank1(dst, (void *)&ptr[rd], sizeof(C));
  }
  rd++;
  if (rd == len) {
    rd = 0;
  }
  CLEAR_LOCK();
  return true;
}

template <class C, int N, class T> C CRingBuffer<C, N, T>::peek() volatile {
  if (isEmpty())
    return (C)0;
  else
    return buf[rd];
}

template <class C, int N, class T>
bool CRingBuffer<C, N, T>::isEmpty() volatile {
  USE_LOCK();
  SET_LOCK();
  bool ret = (rd == wr);
  CLEAR_LOCK();
  return ret;
}

template <class C, int N, class T>
bool CRingBuffer<C, N, T>::isFull() volatile {
  USE_LOCK();
  SET_LOCK();
  T a = wr + 1;
  if (a == len) {
    a = 0;
  }
  bool ret = (a == rd);
  CLEAR_LOCK();
  return ret;
}

/* @} @} */

#endif /* RINGBUFFER_H__ */
