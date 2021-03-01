/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef RINGBUFFER_H__
#define RINGBUFFER_H__

#include "WProgram.h"
#include <inttypes.h>

#ifdef DEBUGMODE
#define CHECKING
#endif

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
  // NOTE keep read/write head as volatile to prevent
  // the compiler from re-ordering the access around
  // SET_LOCK() and CLEAR_LOCK()
  volatile T rd, wr;
  T len = N;
  C *ptr = NULL;
  C buf[N];
  #ifdef CHECKING
  volatile uint8_t overflow;
  #endif
  CRingBuffer(uint8_t *ptr = NULL);
  /** Add a new element c to the ring buffer. **/
  ALWAYS_INLINE() void put(C c);
  /** copy n elements from src buffer to ring buffer **/
  ALWAYS_INLINE() void put(C *src, T n);
  /** put_h but when running from within isr that is already blocking**/
  ALWAYS_INLINE() void put_h_isr(C c);
  /** put_h in isr, copy n elements from src buffer to ring buffer **/
  ALWAYS_INLINE() void put_h_isr(C *src, T n);
  /** Copy a new element pointed to by c to the ring buffer. **/
  ALWAYS_INLINE() void putp(C *c);
  /** Return the next element in the ring buffer. **/
  ALWAYS_INLINE() C get();
  /** get_h but when running from within isr that is already blocking**/
  ALWAYS_INLINE() C get_h_isr();
  /** Copy the next element into dst. **/
  ALWAYS_INLINE() void getp(C *dst);
  /** Get the next element without removing it from the ring buffer. **/
  ALWAYS_INLINE() C peek();
  /** Returns true if the ring buffer is empty. **/
  ALWAYS_INLINE() bool isEmpty();
  /** Returns true if the ring buffer is empty. Use in isr**/
  ALWAYS_INLINE() bool isEmpty_isr();
  /** Returns true if the ring buffer is full. **/
  ALWAYS_INLINE() bool isFull();
  /** Returns the number of elements in the ring buffer. **/
  T size();

  /* @} */
};

template <int N, class T = uint8_t>
class RingBuffer : public CRingBuffer<uint8_t, N, T> {
public:
  RingBuffer(){};
};

template <class C, int N, class T>
CRingBuffer<C, N, T>::CRingBuffer(uint8_t *_ptr) {
  ptr = (C*)_ptr;
  rd = 0;
  wr = 0;
  #ifdef CHECKING
  overflow = 0;
  #endif
}

template <class C, int N, class T>
void CRingBuffer<C, N, T>::put_h_isr(C *src, T n) {
  #ifdef CHECKING
  if (isFull()) {
    overflow++;
    return false;
  }
  #endif

  T s = n;

  if (wr + n > len) {
    s = len -  wr;
  }
  if constexpr (N == 0) {
    memcpy_bank1(ptr + wr, src, s * sizeof(C));
  } else {
    memcpy(buf + wr, src, s * sizeof(C));
  }
  wr += s;
  n -= s;
  if (n) {
    if constexpr (N == 0) {
      memcpy_bank1(ptr, src + s, n * sizeof(C));
    } else {
      memcpy(buf, src + s, n * sizeof(C));
    }
    wr = n;
  }
  if (wr == len) {
    wr = 0;
  }
}

template <class C, int N, class T>
void CRingBuffer<C, N, T>::put_h_isr(C c) {
  #ifdef CHECKING
  if (isFull()) {
    overflow++;
    return false;
  }
  #endif

  if constexpr (N == 0) {
    put_bank1(ptr + wr, c);
  } else {
    buf[wr] = c;
  }
  wr++;
  if (wr == len) {
    wr = 0;
  }
}

template <class C, int N, class T>
void CRingBuffer<C, N, T>::put(C *src, T n) {
  USE_LOCK();
  SET_LOCK();
  put_h_isr(src, n);
  CLEAR_LOCK();
}


template <class C, int N, class T>
void CRingBuffer<C, N, T>::put(C c) {
  USE_LOCK();
  SET_LOCK();
  put_h_isr(c);
  CLEAR_LOCK();
}

template <class C, int N, class T> T CRingBuffer<C, N, T>::size() {
  if (wr >= rd) {
    return wr - rd;
  } else {
    return 256 << (sizeof(T) - 1) - rd + wr;
  }
}

template <class C, int N, class T>
void CRingBuffer<C, N, T>::putp(C *c) {
  USE_LOCK();
  SET_LOCK();
  put_h_isr(c, 1);
  CLEAR_LOCK();
}

template <class C, int N, class T> C CRingBuffer<C, N, T>::get_h_isr() {
  C ret;
  if (isEmpty_isr())
    return ret;
  if constexpr (N == 0) {
    ret = get_bank1(ptr + rd);
  } else {
    ret = buf[rd];
  }
  rd++;
  if (rd == len) {
    rd = 0;
  }
  return ret;
}


template <class C, int N, class T> C CRingBuffer<C, N, T>::get() {
  USE_LOCK();
  SET_LOCK();
  C ret = get_h_isr();
  CLEAR_LOCK();
  return ret;
}

template <class C, int N, class T>
void CRingBuffer<C, N, T>::getp(C *dst) {
  USE_LOCK();
  SET_LOCK();
  C v = get_h_isr();
  CLEAR_LOCK();
  *dst = v;
}

template <class C, int N, class T> C CRingBuffer<C, N, T>::peek() {
  if (isEmpty())
    return (C)0;
  else
    return buf[rd];
}

template <class C, int N, class T>
bool CRingBuffer<C, N, T>::isEmpty() {
  USE_LOCK();
  SET_LOCK();
  bool ret = (rd == wr);
  CLEAR_LOCK();
  return ret;
}

template <class C, int N, class T>
bool CRingBuffer<C, N, T>::isEmpty_isr() {
  bool ret = (rd == wr);
  return ret;
}

template <class C, int N, class T>
bool CRingBuffer<C, N, T>::isFull() {
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
