// WString.h — shared shim. MCL doesn't use the Arduino String class in
// core paths, but a few helper headers expect the type to exist.
//
// On desktop we alias to std::string (full Arduino String surface). On
// wasm32 (no libc++) we use a tiny non-owning String substitute covering
// the few operations MCL touches (c_str, length, equality with cstring).
#pragma once

#if defined(__cplusplus) && !defined(PLATFORM_WASM)

#include <string>
using String = std::string;

// Arduino's String concatenation operator returns a StringSumHelper rather
// than String. MCL's DebugBuffer overloads put(const StringSumHelper&), so
// the type must be implicitly convertible to std::string.
class StringSumHelper {
public:
    StringSumHelper(const std::string& s) : s_(s) {}
    StringSumHelper(const char* s) : s_(s ? s : "") {}
    operator std::string() const { return s_; }
    const char* c_str() const { return s_.c_str(); }
private:
    std::string s_;
};

#elif defined(__cplusplus) && defined(PLATFORM_WASM)

// wasm32 / freestanding — no libc++. Tiny non-owning String covers what
// MCL actually touches.
class String {
public:
    String() : data_("") {}
    String(const char* s) : data_(s ? s : "") {}
    const char* c_str() const { return data_; }
    unsigned    length() const {
        unsigned n = 0; while (data_[n]) ++n; return n;
    }
    bool operator==(const char* o) const {
        const char* a = data_; while (*a && *o && *a == *o) { ++a; ++o; }
        return *a == 0 && *o == 0;
    }
private:
    const char* data_;
};

class StringSumHelper {
public:
    StringSumHelper(const char* s) : s_(s ? s : "") {}
    operator String() const { return String(s_); }
    const char* c_str() const { return s_; }
private:
    const char* s_;
};

#endif
