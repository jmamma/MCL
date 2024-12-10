#pragma once

#include "WProgram.h"
#include <inttypes.h>

#ifdef DEBUGMODE
#define CHECKING
#endif

class RingBuffer {
public:
    volatile uint16_t rd;
    volatile uint16_t wr;
    volatile uint16_t len;
    volatile uint8_t* buf;
    #ifdef CHECKING
    volatile uint8_t overflow;
    bool check = true;
    #endif

    RingBuffer(uint8_t* buffer, uint16_t size) {
        buf = buffer;
        len = size;
        init();
    }

    ALWAYS_INLINE() void init() volatile {
        noInterrupts();
        rd = 0;
        wr = 0;
        #ifdef CHECKING
        overflow = 0;
        #endif
        interrupts();
    }

    /** copy n elements from src buffer to ring buffer **/
    ALWAYS_INLINE() void put_h_isr(uint8_t* src, uint16_t n) volatile {
        #ifdef CHECKING
        if (isFull_isr() && check) {
            overflow++;
            return;
        }
        #endif

        uint16_t s = n;
        if (wr + n > len) {
            s = len - wr;
        }
        memcpy((void*)(buf + wr), src, s);
        wr += s;
        n -= s;
        if (n) {
            memcpy((void*)buf, src + s, n);
            wr = n;
        }
        if (wr == len) {
            wr = 0;
        }
    }

    /** put_h but when running from within isr that is already blocking **/
    ALWAYS_INLINE() void put_h_isr(uint8_t c) volatile {
        #ifdef CHECKING
        if (isFull_isr() && check) {
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
    ALWAYS_INLINE() void get_h_isr(uint8_t* dst, uint16_t n) volatile {
        uint16_t s = n;
        if (rd + n > len) {
            s = len - rd;
        }
        memcpy(dst, (void*)(buf + rd), s);
        rd += s;
        n -= s;
        if (n) {
            memcpy(dst + s, (void*)buf, n);
            rd = n;
        }
        if (rd == len) {
            rd = 0;
        }
    }

    /** Add a new element c to the ring buffer **/
    ALWAYS_INLINE() void put(uint8_t c) volatile {
        noInterrupts();
        put_h_isr(c);
        interrupts();
    }

    /** Copy n elements from src buffer to ring buffer **/
    ALWAYS_INLINE() void put(uint8_t* src, uint16_t n) volatile {
        noInterrupts();
        put_h_isr(src, n);
        interrupts();
    }

    /** Return the next element in the ring buffer **/
    ALWAYS_INLINE() uint8_t get_h_isr() volatile {
        uint8_t ret;
        #ifdef CHECKING
        if (isEmpty_isr())
            return 0;
        #endif

        ret = buf[rd];
        rd++;
        if (rd == len) {
            rd = 0;
        }
        return ret;
    }

    ALWAYS_INLINE() uint8_t get() volatile {
        noInterrupts();
        uint8_t ret = get_h_isr();
        interrupts();
        return ret;
    }

    /** Copy a new element pointed to by c to the ring buffer **/
    ALWAYS_INLINE() void putp(uint8_t* c) volatile {
        noInterrupts();
        put_h_isr(c, 1);
        interrupts();
    }

    /** Copy the next element into dst **/
    ALWAYS_INLINE() void getp(uint8_t* dst) volatile {
        noInterrupts();
        uint8_t v = get_h_isr();
        interrupts();
        *dst = v;
    }

    /** Get the next element without removing it from the ring buffer **/
    ALWAYS_INLINE() uint8_t peek() volatile {
        if (isEmpty())
            return 0;
        return buf[rd];
    }

    /** Returns true if the ring buffer is empty **/
    ALWAYS_INLINE() bool isEmpty() volatile {
        noInterrupts();
        bool ret = (rd == wr);
        interrupts();
        return ret;
    }

    /** Returns true if the ring buffer is empty. Use in isr **/
    ALWAYS_INLINE() bool isEmpty_isr() volatile {
        return (rd == wr);
    }

    /** Returns true if the ring buffer is full **/
    ALWAYS_INLINE() bool isFull() volatile {
        noInterrupts();
        uint16_t a = wr + 1;
        if (a == len) {
            a = 0;
        }
        bool ret = (a == rd);
        interrupts();
        return ret;
    }

    /** Returns true if the ring buffer is full. Use in isr **/
    ALWAYS_INLINE() bool isFull_isr() volatile {
        uint16_t a = wr + 1;
        if (a == len) {
            a = 0;
        }
        return (a == rd);
    }

    /** Returns the number of elements in the ring buffer **/
    uint16_t size() volatile {
        if (wr >= rd) {
            return wr - rd;
        } else {
            return len - rd + wr;
        }
    }
};
