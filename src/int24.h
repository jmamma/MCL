#pragma once

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

    bool is_negative() const {
        return (bytes[2] & 0x80) != 0;
    }

    uint8_t* raw() { return bytes; }
    const uint8_t* raw() const { return bytes; }

    // Basic arithmetic operators
    __int24& operator+=(int32_t value) {
        *this = __int24(int32_t(*this) + value);
        return *this;
    }

    __int24& operator-=(int32_t value) {
        *this = __int24(int32_t(*this) - value);
        return *this;
    }

    __int24& operator*=(int32_t value) {
        *this = __int24(int32_t(*this) * value);
        return *this;
    }
};
#endif
