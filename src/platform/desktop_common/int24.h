// int24.h — desktop copy of the portable __int24 emulation. Lifted from
// MCL/src/platform/rp2040/int24.h verbatim because that file is already
// platform-agnostic but lives outside our include path.
#pragma once

#include <stdint.h>

#if !defined(__AVR__) && !defined(__int24)
class __int24 {
private:
    uint8_t bytes[3];  // LSB first

public:
    __int24() : bytes{0, 0, 0} {}

    __int24(int32_t value) {
        bytes[0] = value & 0xFF;
        bytes[1] = (value >> 8) & 0xFF;
        bytes[2] = (value >> 16) & 0xFF;
    }

    operator int32_t() const {
        int32_t value = ((bytes[2] << 16) | (bytes[1] << 8) | bytes[0]);
        if (bytes[2] & 0x80) {
            value |= 0xFF000000;
        }
        return value;
    }

    __int24& operator=(int32_t value) {
        bytes[0] = value & 0xFF;
        bytes[1] = (value >> 8) & 0xFF;
        bytes[2] = (value >> 16) & 0xFF;
        return *this;
    }

    bool is_negative() const { return (bytes[2] & 0x80) != 0; }

    uint8_t* raw() { return bytes; }
    const uint8_t* raw() const { return bytes; }

    __int24& operator+=(int32_t value) { *this = __int24(int32_t(*this) + value); return *this; }
    __int24& operator-=(int32_t value) { *this = __int24(int32_t(*this) - value); return *this; }
    __int24& operator*=(int32_t value) { *this = __int24(int32_t(*this) * value); return *this; }

    // Unary minus. Without this, `-__int24` implicitly converts to int32_t,
    // and the `abs(x)` macro (`x < 0 ? -x : x`) produces a ternary with
    // int32_t and __int24 branches that clang flags as ambiguous.
    __int24 operator-() const { return __int24(-int32_t(*this)); }
};

// Comparison overloads. Without these the implicit-conversion paths
// (int -> __int24 via ctor, __int24 -> int via operator) are equally good
// when comparing __int24 with int, and clang flags MCL's Wav.cpp as
// ambiguous. Provide all three combinations explicitly so overload
// resolution always has a uniquely best match.
inline bool operator<(const __int24& a, const __int24& b)  { return int32_t(a) <  int32_t(b); }
inline bool operator>(const __int24& a, const __int24& b)  { return int32_t(a) >  int32_t(b); }
inline bool operator<=(const __int24& a, const __int24& b) { return int32_t(a) <= int32_t(b); }
inline bool operator>=(const __int24& a, const __int24& b) { return int32_t(a) >= int32_t(b); }
inline bool operator==(const __int24& a, const __int24& b) { return int32_t(a) == int32_t(b); }
inline bool operator!=(const __int24& a, const __int24& b) { return int32_t(a) != int32_t(b); }

inline bool operator<(const __int24& a, int32_t b)  { return int32_t(a) <  b; }
inline bool operator>(const __int24& a, int32_t b)  { return int32_t(a) >  b; }
inline bool operator<=(const __int24& a, int32_t b) { return int32_t(a) <= b; }
inline bool operator>=(const __int24& a, int32_t b) { return int32_t(a) >= b; }
inline bool operator==(const __int24& a, int32_t b) { return int32_t(a) == b; }
inline bool operator!=(const __int24& a, int32_t b) { return int32_t(a) != b; }

inline bool operator<(int32_t a, const __int24& b)  { return a <  int32_t(b); }
inline bool operator>(int32_t a, const __int24& b)  { return a >  int32_t(b); }
inline bool operator<=(int32_t a, const __int24& b) { return a <= int32_t(b); }
inline bool operator>=(int32_t a, const __int24& b) { return a >= int32_t(b); }
inline bool operator==(int32_t a, const __int24& b) { return a == int32_t(b); }
inline bool operator!=(int32_t a, const __int24& b) { return a != int32_t(b); }
#endif
