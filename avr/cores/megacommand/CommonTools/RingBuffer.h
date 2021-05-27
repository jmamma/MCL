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
  bool check = true;
  #endif
  CRingBuffer(volatile uint8_t *ptr = NULL);
  /** Reset the buffer **/
  ALWAYS_INLINE() void init() volatile;
  /** Add a new element c to the ring buffer. **/
  ALWAYS_INLINE() void put(C c) volatile;
  /** copy n elements from src buffer to ring buffer **/
  ALWAYS_INLINE() void put(C *src, T n) volatile;
  /** put_h but when running from within isr that is already blocking**/
  ALWAYS_INLINE() void put_h_isr(C c) volatile;
  /** put_h in isr, copy n elements from src buffer to ring buffer **/
  ALWAYS_INLINE() void put_h_isr(C *src, T n) volatile;
  /** get_h in isr, copy n elements from src buffer to ring buffer **/
  ALWAYS_INLINE() void get_h_isr(C *dst, T n) volatile;
  /** Copy a new element pointed to by c to the ring buffer. **/
  ALWAYS_INLINE() void putp(C *c) volatile;
  /** Return the next element in the ring buffer. **/
  ALWAYS_INLINE() C get() volatile;
  /** get_h but when running from within isr that is already blocking**/
  ALWAYS_INLINE() C get_h_isr() volatile;
  /** Copy the next element into dst. **/
  ALWAYS_INLINE() void getp(C *dst) volatile;
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
  init();
}

template <class C, int N, class T>
void CRingBuffer<C, N, T>::init() volatile {
  USE_LOCK();
  SET_LOCK();
  rd = 0;
  wr = 0;
  #ifdef CHECKING
  overflow = 0;
  #endif
  CLEAR_LOCK();
}

template <class C, int N, class T>
void CRingBuffer<C, N, T>::get_h_isr(C *dst, T n) volatile {
  #ifdef CHECKING
  if (isFull() && check) {
    overflow++;
    return false;
  }
  #endif

  T s = n;

  if (rd + n > len) {
    s = len -  rd;
  }
  if constexpr (N == 0) {
    memcpy_bank1(dst, ptr + rd, s * sizeof(C));
  } else {
    memcpy(dst, buf + rd, s * sizeof(C));
  }
  rd += s;
  n -= s;
  if (n) {
    if constexpr (N == 0) {
      memcpy_bank1(dst + s, ptr, n * sizeof(C));
    } else {
      memcpy(dst + s, buf, n * sizeof(C));
    }
    rd = n;
  }
  if (rd == len) {
    rd = 0;
  }
}


template <class C, int N, class T>
void CRingBuffer<C, N, T>::put_h_isr(C *src, T n) volatile {
  #ifdef CHECKING
  if (isFull() && check) {
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
      memcpy(buf ,src + s, n * sizeof(C));
    }
    wr = n;
  }
  if (wr == len) {
    wr = 0;
  }
}



template <class C, int N, class T>
void CRingBuffer<C, N, T>::put_h_isr(C c) volatile {
  #ifdef CHECKING
  if (isFull() && check) {
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
void CRingBuffer<C, N, T>::put(C *src, T n) volatile {
  USE_LOCK();
  SET_LOCK();
  put_h_isr(src, n);
  CLEAR_LOCK();
}


template <class C, int N, class T>
void CRingBuffer<C, N, T>::put(C c) volatile {
  USE_LOCK();
  SET_LOCK();
  put_h_isr(c);
  CLEAR_LOCK();
}

template <class C, int N, class T> T CRingBuffer<C, N, T>::size() volatile {
  if (wr >= rd) {
    return wr - rd;
  } else {
    return len - rd + wr;
  }
}

template <class C, int N, class T>
void CRingBuffer<C, N, T>::putp(C *c) volatile {
  USE_LOCK();
  SET_LOCK();
  put_h_isr(c, 1);
  CLEAR_LOCK();
}

template <class C, int N, class T> C CRingBuffer<C, N, T>::get_h_isr() volatile {
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


template <class C, int N, class T> C CRingBuffer<C, N, T>::get() volatile {
  USE_LOCK();
  SET_LOCK();
  C ret = get_h_isr();
  CLEAR_LOCK();
  return ret;
}

template <class C, int N, class T>
void CRingBuffer<C, N, T>::getp(C *dst) volatile {
  USE_LOCK();
  SET_LOCK();
  C v = get_h_isr();
  CLEAR_LOCK();
  *dst = v;
}

template <class C, int N, class T> C CRingBuffer<C, N, T>::peek() volatile {
  if (isEmpty())
    return (C)0;
  else if constexpr (N != 0)
    return buf[rd];
  else
    return get_bank1(ptr + rd);
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
