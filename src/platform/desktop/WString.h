// WString.h — desktop shim. MCL doesn't use the Arduino String class in core
// paths, but a few helper headers expect the type to exist. std::string is
// API-compatible for the operations MCL uses (c_str, length, +=, ==).
#pragma once
#include <string>

using String = std::string;

// Arduino's String concatenation operator returns a StringSumHelper rather
// than String to enable expression-template-style chains. MCL's DebugBuffer
// has a `put(const StringSumHelper&)` overload, so the type must be
// implicitly convertible to std::string.
class StringSumHelper {
public:
    StringSumHelper(const std::string& s) : s_(s) {}
    StringSumHelper(const char* s) : s_(s ? s : "") {}
    operator std::string() const { return s_; }
    const char* c_str() const { return s_.c_str(); }
private:
    std::string s_;
};
