#pragma once

#include "WProgram.h"
#include <inttypes.h>
#include "memory.h"

#ifdef DEBUGMODE
#define CHECKING
#endif

template<typename T = uint8_t>
class RingBuffer {
public:
    volatile uint16_t rd;
    volatile uint16_t wr;
    uint16_t len;
    T* buf;
#ifdef CHECKING
    volatile uint8_t overflow;
    bool check = true;
#endif

    RingBuffer(T* buffer, uint16_t size) {
        buf = buffer;
        len = size;
        init();
    }

    ALWAYS_INLINE() void init() volatile {
        LOCK();
        rd = 0;
        wr = 0;
#ifdef CHECKING
        overflow = 0;
#endif
        CLEAR_LOCK();
    }

    ALWAYS_INLINE() void put_h_isr(const T* src, uint16_t n) volatile {
#ifdef CHECKING
        if (isFull_isr(n) && check) {
            overflow++;
            DEBUG_PRINTLN("overflow");
            return;
        }
#endif

        uint16_t s = n;
        if (wr + n > len) {
            s = len - wr;
        }
        memcpy_bank1((void*)(buf + wr), src, s * sizeof(T));
        wr += s;
        n -= s;
        if (n) {
            memcpy_bank1((void*)buf, src + s, n * sizeof(T));
            wr = n;
        }
        if (wr == len) {
            wr = 0;
        }
    }

    ALWAYS_INLINE() void put_h_isr(T c) volatile {
#ifdef CHECKING
        if (isFull_isr() && check) {
            overflow++;
            DEBUG_PRINTLN("overflow");
            return;
        }
#endif
        buf[wr] = c;
        wr++;
        if (wr == len) {
            wr = 0;
        }
    }

    ALWAYS_INLINE() void get_h_isr(T* dst, uint16_t n) volatile {
        uint16_t s = n;
        if (rd + n > len) {
            s = len - rd;
        }
        memcpy_bank1(dst, (void*)(buf + rd), s * sizeof(T));
        rd += s;
        n -= s;
        if (n) {
            memcpy_bank1(dst + s, (void*)buf, n * sizeof(T));
            rd = n;
        }
        if (rd == len) {
            rd = 0;
        }
    }

    ALWAYS_INLINE() void put(T c) volatile {
        LOCK();
        put_h_isr(c);
        CLEAR_LOCK();
    }

    ALWAYS_INLINE() void put(const T* src, uint16_t n) volatile {
        LOCK();
        put_h_isr(src, n);
        CLEAR_LOCK();
    }

    ALWAYS_INLINE() T get_h_isr() volatile {
        T ret;
#ifdef CHECKING
        if (isEmpty_isr()) {
            DEBUG_PRINTLN("buffer empty");
            return T();
        }
#endif
        ret = buf[rd];
        rd++;
        if (rd == len) {
            rd = 0;
        }
        return ret;
    }

    ALWAYS_INLINE() T get() volatile {
        LOCK();
        T ret = get_h_isr();
        CLEAR_LOCK();
        return ret;
    }

    ALWAYS_INLINE() void putp(const T* c) volatile {
        LOCK();
        put_h_isr(c, 1);
        CLEAR_LOCK();
    }

    ALWAYS_INLINE() void getp(T* dst) volatile {
        LOCK();
        *dst = get_h_isr();
        CLEAR_LOCK();
    }

    ALWAYS_INLINE() T peek() volatile {
        LOCK();
        if (isEmpty_isr()) {
            CLEAR_LOCK();
            return T();
        }
        T ret = buf[rd];
        CLEAR_LOCK();
        return ret;
    }

    ALWAYS_INLINE() bool isEmpty() volatile {
        LOCK();
        bool ret = (rd == wr);
        CLEAR_LOCK();
        return ret;
    }

    ALWAYS_INLINE() bool isEmpty_isr() volatile {
        return (rd == wr);
    }

    ALWAYS_INLINE() bool isFull(uint16_t n = 1) volatile {
        LOCK();
        bool ret = isFull_isr(n);
        CLEAR_LOCK();
        return ret;
    }

    ALWAYS_INLINE() bool isFull_isr(uint16_t n = 1) volatile {
        uint16_t next_wr = wr + n;
        if (next_wr >= len) {
            next_wr -= len;
        }
        return (next_wr == rd);
    }

    ALWAYS_INLINE() uint16_t size() volatile {
        LOCK();
        uint16_t sz;
        if (wr >= rd) {
            sz = wr - rd;
        } else {
            sz = len - rd + wr;
        }
        CLEAR_LOCK();
        return sz;
    }
};

// Helper template class to match the CRingBuffer<T, SIZE> syntax while using external buffer
template<typename T, uint16_t SIZE>
class CRingBuffer : public RingBuffer<T> {
private:
    T buffer[SIZE];
public:
    CRingBuffer() : RingBuffer<T>(buffer, SIZE) {}
};
