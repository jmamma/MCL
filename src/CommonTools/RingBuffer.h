#pragma once

#include "WProgram.h"
#include <inttypes.h>

#ifdef DEBUGMODE
#define CHECKING
#endif

template <class C, uint16_t SIZE, class T = uint8_t>
class CRingBuffer {
public:
   volatile T rd, wr;
   volatile T len;
   volatile C buf[SIZE];
   #ifdef CHECKING
   volatile uint8_t overflow;
   bool check = true;
   #endif

   CRingBuffer() {
       len = SIZE;
       init();
   }

   ALWAYS_INLINE() void init() volatile {
       USE_LOCK();
       SET_LOCK();
       rd = 0;
       wr = 0;
       #ifdef CHECKING
       overflow = 0;
       #endif
       CLEAR_LOCK();
   }

   /** copy n elements from src buffer to ring buffer **/
   ALWAYS_INLINE() void put_h_isr(C *src, T n) volatile {
       #ifdef CHECKING
       if (isFull() && check) {
           overflow++;
           return;
       }
       #endif

       T s = n;
       if (wr + n > len) {
           s = len - wr;
       }
       memcpy((void*)(buf + wr), src, s * sizeof(C));
       wr += s;
       n -= s;
       if (n) {
           memcpy((void*)buf, src + s, n * sizeof(C));
           wr = n;
       }
       if (wr == len) {
           wr = 0;
       }
   }

   /** put_h but when running from within isr that is already blocking **/
   ALWAYS_INLINE() void put_h_isr(C c) volatile {
       #ifdef CHECKING
       if (isFull() && check) {
           overflow++;
           return;
       }
       #endif

       buf[wr] = c;
       wr++;
       if (wr == len) {
           wr = 0;
       }
   }

   /** get_h in isr, copy n elements to dst buffer **/
   ALWAYS_INLINE() void get_h_isr(C *dst, T n) volatile {
       T s = n;
       if (rd + n > len) {
           s = len - rd;
       }
       memcpy(dst, (void*)(buf + rd), s * sizeof(C));
       rd += s;
       n -= s;
       if (n) {
           memcpy(dst + s, (void*)buf, n * sizeof(C));
           rd = n;
       }
       if (rd == len) {
           rd = 0;
       }
   }

   /** Add a new element c to the ring buffer **/
   ALWAYS_INLINE() void put(C c) volatile {
       USE_LOCK();
       SET_LOCK();
       put_h_isr(c);
       CLEAR_LOCK();
   }

   /** Copy n elements from src buffer to ring buffer **/
   ALWAYS_INLINE() void put(C *src, T n) volatile {
       USE_LOCK();
       SET_LOCK();
       put_h_isr(src, n);
       CLEAR_LOCK();
   }

   /** Return the next element in the ring buffer **/
   ALWAYS_INLINE() C get_h_isr() volatile {
       C ret;
       #ifdef CHECKING
       if (isEmpty_isr())
           return (C)0;
       #endif

       ret = buf[rd];
       rd++;
       if (rd == len) {
           rd = 0;
       }
       return ret;
   }

   ALWAYS_INLINE() C get() volatile {
       USE_LOCK();
       SET_LOCK();
       C ret = get_h_isr();
       CLEAR_LOCK();
       return ret;
   }

   /** Copy a new element pointed to by c to the ring buffer **/
   ALWAYS_INLINE() void putp(C *c) volatile {
       USE_LOCK();
       SET_LOCK();
       put_h_isr(c, 1);
       CLEAR_LOCK();
   }

   /** Copy the next element into dst **/
   ALWAYS_INLINE() void getp(C *dst) volatile {
       USE_LOCK();
       SET_LOCK();
       C v = get_h_isr();
       CLEAR_LOCK();
       *dst = v;
   }

   /** Get the next element without removing it from the ring buffer **/
   ALWAYS_INLINE() C peek() volatile {
       if (isEmpty())
           return (C)0;
       return buf[rd];
   }

   /** Returns true if the ring buffer is empty **/
   ALWAYS_INLINE() bool isEmpty() volatile {
       USE_LOCK();
       SET_LOCK();
       bool ret = (rd == wr);
       CLEAR_LOCK();
       return ret;
   }

   /** Returns true if the ring buffer is empty. Use in isr **/
   ALWAYS_INLINE() bool isEmpty_isr() volatile {
       return (rd == wr);
   }

   /** Returns true if the ring buffer is full **/
   ALWAYS_INLINE() bool isFull() volatile {
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

   /** Returns true if the ring buffer is full. Use in isr **/
   ALWAYS_INLINE() bool isFull_isr() volatile {
       T a = wr + 1;
       if (a == len) {
           a = 0;
       }
       return (a == rd);
   }

   /** Returns the number of elements in the ring buffer **/
   T size() volatile {
       if (wr >= rd) {
           return wr - rd;
       } else {
           return len - rd + wr;
       }
   }
};

template <uint16_t SIZE>
class RingBuffer : public CRingBuffer<uint8_t, SIZE, uint16_t> {
public:
   RingBuffer() : CRingBuffer<uint8_t, SIZE, uint16_t>() {};
};
template<>
class RingBuffer<0> : public CRingBuffer<uint8_t, 0, uint16_t> {
public:
    RingBuffer() : CRingBuffer<uint8_t, 0, uint16_t>() {};
};
