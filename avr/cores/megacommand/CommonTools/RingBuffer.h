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
  CRingBuffer(const CRingBuffer<C,N,T>& other);
  /** Add a new element c to the ring buffer. **/
  ALWAYS_INLINE() bool put(C c) volatile;
  /** A slightly more efficient version of put, if ptr == NULL */
  ALWAYS_INLINE() bool put_h(C c) volatile;
  /** put_h but when running from within isr that is already blocking**/
  ALWAYS_INLINE() bool put_h_isr(C c) volatile;
  /** Copy a new element pointed to by c to the ring buffer. **/
  ALWAYS_INLINE() bool putp(C *c) volatile;
  /** Drop _at most_ the next n element in the ring buffer. **/
  ALWAYS_INLINE() void skip(T n) volatile;
  /** Drop _at most_ the most recent n element in the ring buffer. **/
  ALWAYS_INLINE() void undo(T n) volatile;
  /** Return the next element in the ring buffer. **/
  ALWAYS_INLINE() C get() volatile;
  /** A slightly more efficient version of get, if ptr == NULL */
  ALWAYS_INLINE() C get_h() volatile;
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
  /** Returns true if the ring buffer is full. Use in isr **/
  ALWAYS_INLINE() bool isFull_isr() volatile;
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
CRingBuffer<C, N, T>::CRingBuffer(const CRingBuffer<C, N, T>& other) {
  ptr = other.ptr;
  rd = other.rd;
  wr = other.wr;
  #ifdef CHECKING
  overflow = other.overflow;
  #endif
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
bool CRingBuffer<C, N, T>::put_h(C c) volatile {
  USE_LOCK();
  SET_LOCK();
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
    return len - rd + wr;
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

template <class C, int N, class T> C CRingBuffer<C, N, T>::get_h() volatile {
  USE_LOCK();
  SET_LOCK();
  C ret;
  if (isEmpty_isr()) {
    return ret;
  }
  ret = get_bank1(ptr + rd);
  rd++;
  if (rd == len) {
    rd = 0;
  }
  CLEAR_LOCK();
  return ret;
}


template <class C, int N, class T> C CRingBuffer<C, N, T>::get_h_isr() volatile {
  C ret;
  if (isEmpty_isr()) {
    return ret;
  }
  ret = get_bank1(ptr + rd);
  rd++;
  if (rd == len) {
    rd = 0;
  }
  return ret;
}

template <class C, int N, class T> void CRingBuffer<C, N, T>::skip(T n) volatile {
  USE_LOCK();
  SET_LOCK();
  auto sz = size();
  if (n > sz) n = sz;
  unsigned long rd_new = rd;
  rd_new += n;
  if(rd_new >= len) {
    rd_new -= len;
  }
  rd = rd_new;
  CLEAR_LOCK();
}

template <class C, int N, class T> void CRingBuffer<C, N, T>::undo(T n) volatile {
  USE_LOCK();
  SET_LOCK();
  auto sz = size();
  if (n > sz) n = sz;
  int wr_new = wr;
  wr_new -= n;
  if(wr_new < 0) {
    wr_new += len;
  }
  wr = wr_new;
  CLEAR_LOCK();
}

template <class C, int N, class T> C CRingBuffer<C, N, T>::get() volatile {
  USE_LOCK();
  SET_LOCK();
  C ret;
  if (isEmpty_isr()) {
    return ret;
  }
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

template <class C, int N, class T>
bool CRingBuffer<C, N, T>::isFull_isr() volatile {
  T a = wr + 1;
  if (a == len) {
    a = 0;
  }
  bool ret = (a == rd);
  return ret;
}

/* @} @} */

#endif /* RINGBUFFER_H__ */
