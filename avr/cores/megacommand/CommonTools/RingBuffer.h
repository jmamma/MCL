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
  volatile T rd, wr;
  volatile C buf[N];
  volatile T len = N;
  volatile C *ptr = NULL;
  #ifdef CHECKING
  volatile uint8_t overflow;
  #endif
  CRingBuffer(volatile uint8_t *ptr = NULL);
  /** Add a new element c to the ring buffer. **/
  ALWAYS_INLINE() bool put(C c) volatile;
  /** copy n elements from src buffer to ring buffer **/
  ALWAYS_INLINE() bool put(C *src, T n) volatile;
  /** put_h but when running from within isr that is already blocking**/
  ALWAYS_INLINE() bool put_h_isr(C c) volatile;
  /** put_h in isr, copy n elements from src buffer to ring buffer **/
  ALWAYS_INLINE() bool put_h_isr(C *src, T n) volatile;
  /** Copy a new element pointed to by c to the ring buffer. **/
  ALWAYS_INLINE() bool putp(C *c) volatile;
  /** Return the next element in the ring buffer. **/
  ALWAYS_INLINE() C get() volatile;
  /** get_h but when running from within isr that is already blocking**/
  ALWAYS_INLINE() C get_h_isr() volatile;
  /** Copy the next element into dst. **/
  ALWAYS_INLINE() bool getp(C *dst) volatile;
  /** Get the next element without removing it from the ring buffer. **/
  ALWAYS_INLINE() C peek() volatile;
  /** Returns true if the ring buffer is empty. **/
  ALWAYS_INLINE() bool isEmpty() volatile;
  /** Returns true if the ring buffer is empty. Use in isr**/
  ALWAYS_INLINE() bool isEmpty_isr() volatile;
  /** Returns true if the ring buffer is full. **/
  ALWAYS_INLINE() bool isFull() volatile;
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
  #ifdef CHECKING
  overflow = 0;
  #endif
}

template <class C, int N, class T>
bool CRingBuffer<C, N, T>::put_h_isr(C *src, T n) volatile {
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
  memcpy_bank1(ptr + wr, src, s);
  wr += s;
  n -= s;
  if (n) {
    memcpy_bank1(ptr, src + s, n);
    wr = n;
  }
  if (wr == len) {
    wr = 0;
  }
  return true;
}



template <class C, int N, class T>
bool CRingBuffer<C, N, T>::put_h_isr(C c) volatile {
  #ifdef CHECKING
  if (isFull()) {
    overflow++;
    return false;
  }
  #endif

  put_bank1(ptr + wr, c);
  wr++;
  if (wr == len) {
    wr = 0;
  }
  return true;
}

template <class C, int N, class T>
bool CRingBuffer<C, N, T>::put(C *src, T n) volatile {
  USE_LOCK();
  SET_LOCK();
  #ifdef CHECKING
  if (isFull()) {
    overflow++;
    return false;
  }
  #endif

  T s = n;

  if (wr + n > len) {
    s = len - wr;
  }

  if constexpr (N != 0) {
    memcpy(buf + wr, src, s);
  } else {
    memcpy_bank1(ptr + wr, src, s);
  }

  wr += s;
  n -= s;
  if (n) {
    if constexpr (N != 0) {
      memcpy(buf, src + s, n);
    } else {
      memcpy_bank1(ptr, src + s, n);
    }

    wr = n;
  }
  if (wr == len) {
    wr = 0;
  }
  CLEAR_LOCK();
  return true;
}


template <class C, int N, class T>
bool CRingBuffer<C, N, T>::put(C c) volatile {
  USE_LOCK();
  SET_LOCK();
  #ifdef CHECKING
  if (isFull()) {
    overflow++;
    return false;
  }
  #endif
  if constexpr (N != 0) {
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
  USE_LOCK();
  SET_LOCK();
  #ifdef CHECKING
  if (isFull()) {
    overflow++;
    return false;
  }
  #endif
  if constexpr (N != 0) {
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

template <class C, int N, class T> C CRingBuffer<C, N, T>::get_h_isr() volatile {
  if (isEmpty_isr())
    return 0;
  C ret;
  ret = get_bank1(ptr + rd);
  rd++;
  if (rd == len) {
    rd = 0;
  }
  return ret;
}


template <class C, int N, class T> C CRingBuffer<C, N, T>::get() volatile {
  USE_LOCK();
  SET_LOCK();
  if (isEmpty_isr())
    return 0;
  C ret;
  if constexpr (N != 0) {
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
  USE_LOCK();
  SET_LOCK();
  if (isEmpty())
    return false;
  if constexpr(N!=0) {
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
bool CRingBuffer<C, N, T>::isEmpty_isr() volatile {
  bool ret = (rd == wr);
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
