// Wire.h — desktop shim. MCL's core paths don't use I2C; the Adafruit display
// constructors take a TwoWire* but we route through SPI, so this is an empty
// stub kept only so #include <Wire.h> resolves.
#pragma once

#include "Arduino.h"

class TwoWire {
public:
    void begin() {}
    void end() {}
    void setClock(uint32_t /*hz*/) {}
    void beginTransmission(uint8_t /*addr*/) {}
    uint8_t endTransmission(bool /*stop*/ = true) { return 0; }
    size_t write(uint8_t /*b*/)                 { return 1; }
    size_t write(const uint8_t* /*b*/, size_t n){ return n; }
    int  available()  { return 0; }
    int  read()       { return -1; }
    void requestFrom(uint8_t /*addr*/, uint8_t /*qty*/) {}
};

extern TwoWire Wire;
