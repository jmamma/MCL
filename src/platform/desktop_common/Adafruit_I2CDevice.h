// Adafruit_I2CDevice.h — stub. Adafruit_GFX.h includes this transitively.
// We don't use I2C on desktop; the class only needs to exist as a type so
// Adafruit_GFX's pointer members compile.
#pragma once

#include "Arduino.h"
#include "Wire.h"

class Adafruit_I2CDevice {
public:
    Adafruit_I2CDevice(uint8_t /*addr*/, TwoWire* /*theWire*/ = &Wire) {}
    bool begin(bool /*addr_detect*/ = true) { return false; }
    bool detected() { return false; }
    bool read(uint8_t* /*buf*/, size_t /*len*/, bool /*stop*/ = true) { return false; }
    bool write(const uint8_t* /*buf*/, size_t /*len*/, bool /*stop*/ = true,
               const uint8_t* /*prefix*/ = nullptr, size_t /*prefix_len*/ = 0) { return false; }
    bool write_then_read(const uint8_t* /*w*/, size_t /*wl*/,
                         uint8_t* /*r*/, size_t /*rl*/, bool /*stop*/ = false) { return false; }
};
