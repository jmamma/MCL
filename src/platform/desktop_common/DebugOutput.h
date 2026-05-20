// DebugOutput.h — stdio-backed sink for MCL DEBUG_* macros.
//
// On native desktop this writes to stderr. On wasm, the local stdio shim
// forwards fprintf/fputs/fputc to host_log(), so the plugin receives the same
// output without every MCL callsite knowing about WAMR.
#pragma once

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>

#include "WString.h"

namespace mcl_debug {

inline FILE* stream() { return stderr; }

inline void raw(const char* s) {
    fputs(s ? s : "(null)", stream());
}

inline void print(const char* s) { raw(s); }
inline void print(char* s) { raw(s); }
inline void print(char c) { fputc(c, stream()); }
inline void print(bool v) { raw(v ? "1" : "0"); }

inline void print(signed char v) { fprintf(stream(), "%d", (int)v); }
inline void print(unsigned char v) { fprintf(stream(), "%u", (unsigned)v); }
inline void print(short v) { fprintf(stream(), "%d", (int)v); }
inline void print(unsigned short v) { fprintf(stream(), "%u", (unsigned)v); }
inline void print(int v) { fprintf(stream(), "%d", v); }
inline void print(unsigned int v) { fprintf(stream(), "%u", v); }
inline void print(long v) { fprintf(stream(), "%ld", v); }
inline void print(unsigned long v) { fprintf(stream(), "%lu", v); }
inline void print(long long v) { fprintf(stream(), "%lld", v); }
inline void print(unsigned long long v) { fprintf(stream(), "%llu", v); }
inline void print(float v) { fprintf(stream(), "%f", (double)v); }
inline void print(double v) { fprintf(stream(), "%f", v); }
inline void print(const void* p) { fprintf(stream(), "%p", p); }

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
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stream(), fmt ? fmt : "", ap);
    va_end(ap);
    println();
}

}  // namespace mcl_debug
