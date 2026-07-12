// DebugOutput.h — bounded desktop/wasm sink for MCL DEBUG_* macros.
//
// The wasm host_log import is synchronous. Calling it from every DEBUG_PRINT
// site can stall the WAMR service loop when a hot path logs heavily. Match the
// hardware path instead: DEBUG_PRINT* writes into DebugBuffer, and platform
// poll points flush a small budget later. Overflow drops debug bytes.
#pragma once

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>

#include "DebugBuffer.h"
#include "WString.h"

extern DebugBuffer debugBuffer;

namespace mcl_debug {

inline constexpr size_t kFlushBudgetBytes = 512;

inline void raw(const char* s) {
    debugBuffer.put(s ? s : "(null)");
}

inline void formatted(const char* fmt, ...) {
    char buf[96];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt ? fmt : "", ap);
    va_end(ap);
    debugBuffer.put(buf);
}

inline void print(const char* s) { raw(s); }
inline void print(char* s) { raw(s); }
inline void print(char c) { char buf[2] = { c, 0 }; raw(buf); }
inline void print(bool v) { raw(v ? "1" : "0"); }

inline void print(signed char v) { formatted("%d", (int)v); }
inline void print(unsigned char v) { formatted("%u", (unsigned)v); }
inline void print(short v) { formatted("%d", (int)v); }
inline void print(unsigned short v) { formatted("%u", (unsigned)v); }
inline void print(int v) { formatted("%d", v); }
inline void print(unsigned int v) { formatted("%u", v); }
inline void print(long v) { formatted("%ld", v); }
inline void print(unsigned long v) { formatted("%lu", v); }
inline void print(long long v) { formatted("%lld", v); }
inline void print(unsigned long long v) { formatted("%llu", v); }
inline void print(float v) { formatted("%f", (double)v); }
inline void print(double v) { formatted("%f", v); }
inline void print(const void* p) { formatted("%p", p); }

inline void print(const String& s) { raw(s.c_str()); }
inline void print(const StringSumHelper& s) { raw(s.c_str()); }

inline void println() { raw("\n"); }

template <typename T>
inline void println(const T& v) {
    print(v);
    println();
}

inline void function(const char* func) {
    raw(func);
    println();
}

inline void function(const char* func, const char* fmt, ...) {
    raw(func);
    raw(": ");
    char buf[96];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt ? fmt : "", ap);
    va_end(ap);
    raw(buf);
    println();
}

inline void flush(size_t maxBytes = kFlushBudgetBytes) {
    debugBuffer.transmit(maxBytes);
}

}  // namespace mcl_debug
