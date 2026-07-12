// Adafruit_SPIDevice.h — stub. Used only by transitive Adafruit_GFX includes;
// the display drivers we instantiate don't actually use this device wrapper.
#pragma once

#include "Arduino.h"
#include "SPI.h"

// BitOrder is normally defined by ArduinoCore-API; provide it here.
typedef int BitOrder;
typedef BitOrder BusIOBitOrder;

class Adafruit_SPIDevice {
public:
    Adafruit_SPIDevice(int8_t /*cspin*/, uint32_t /*freq*/ = 1000000,
                       BitOrder /*dataOrder*/ = 0, uint8_t /*dataMode*/ = 0,
                       SPIClass* /*theSPI*/ = &SPI) {}
    Adafruit_SPIDevice(int8_t /*cspin*/, int8_t /*sckpin*/, int8_t /*misopin*/,
                       int8_t /*mosipin*/, uint32_t /*freq*/ = 1000000,
                       BitOrder /*dataOrder*/ = 0, uint8_t /*dataMode*/ = 0) {}
    bool begin() { return false; }
    bool read(uint8_t* /*b*/, size_t /*l*/, uint8_t /*sb*/ = 0xFF) { return false; }
    bool write(const uint8_t* /*b*/, size_t /*l*/,
               const uint8_t* /*p*/ = nullptr, size_t /*pl*/ = 0) { return false; }
    bool write_then_read(const uint8_t* /*w*/, size_t /*wl*/,
                         uint8_t* /*r*/, size_t /*rl*/, uint8_t /*sb*/ = 0xFF) { return false; }
    uint8_t transfer(uint8_t /*b*/) { return 0; }
    void transfer(uint8_t* /*b*/, size_t /*l*/) {}

    void beginTransaction() {}
    void endTransaction() {}
    void setChipSelect(int /*v*/) {}
};
