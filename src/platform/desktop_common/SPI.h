// SPI.h — desktop shim.
//
// Adafruit_GFX-derived display drivers expect to instantiate against an
// SPIClass and call beginTransaction/transfer/endTransaction. On desktop we
// don't have a physical bus; the driver computes its framebuffer entirely in
// RAM, then calls transfer() on each flush, which we drop. The plugin reads
// the framebuffer directly via Oled::getBuffer() (see oled.h).
#pragma once

#include "Arduino.h"

#define MSBFIRST 1
#define LSBFIRST 0
#define SPI_MODE0 0
#define SPI_MODE1 1
#define SPI_MODE2 2
#define SPI_MODE3 3

class SPISettings {
public:
    SPISettings() = default;
    SPISettings(uint32_t /*clock*/, uint8_t /*bitOrder*/, uint8_t /*dataMode*/) {}
};

class SPIClass {
public:
    void begin() {}
    void end() {}
    void beginTransaction(SPISettings /*s*/) {}
    void endTransaction() {}

    uint8_t  transfer(uint8_t /*b*/)        { return 0; }
    uint16_t transfer16(uint16_t /*w*/)     { return 0; }
    void     transfer(void* /*buf*/, size_t /*len*/) {}

    void setBitOrder(uint8_t /*order*/) {}
    void setDataMode(uint8_t /*mode*/)  {}
    void setClockDivider(uint8_t /*d*/) {}

    void setSCK(uint8_t /*pin*/)  {}
    void setMOSI(uint8_t /*pin*/) {}
    void setMISO(uint8_t /*pin*/) {}
    void setCS(uint8_t /*pin*/)   {}
};

extern SPIClass SPI;
extern SPIClass SPI1;
