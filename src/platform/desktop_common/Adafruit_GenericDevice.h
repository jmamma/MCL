// Adafruit_GenericDevice.h — stub. Adafruit_BusIO_Register.h includes this
// transitively when read_register/write_register helpers are templated over
// a generic device type.
#pragma once

#include "Arduino.h"

class Adafruit_GenericDevice {
public:
    bool begin() { return false; }
    bool read (uint8_t* /*b*/, size_t /*l*/) { return false; }
    bool write(const uint8_t* /*b*/, size_t /*l*/,
               const uint8_t* /*p*/ = nullptr, size_t /*pl*/ = 0) { return false; }
    bool write_then_read(const uint8_t* /*w*/, size_t /*wl*/,
                         uint8_t* /*r*/, size_t /*rl*/) { return false; }
    size_t maxBufferSize() { return 32; }
};
