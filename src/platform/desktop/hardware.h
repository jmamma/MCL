#pragma once

// hardware.h stub for desktop builds

#include <stdint.h>

// Hardware definitions (stubs)
#define SD_CS 0
#define OLED_CS 0
#define OLED_DC 0
#define OLED_RST 0

// SPI stubs
class SPIClass {
public:
    void begin() {}
    void end() {}
    void beginTransaction(uint32_t speed) {}
    void endTransaction() {}
    uint8_t transfer(uint8_t data) { return 0; }
};

extern SPIClass SPI;

// I2C stubs
class TwoWire {
public:
    void begin() {}
    void beginTransmission(uint8_t addr) {}
    uint8_t endTransmission() { return 0; }
    void write(uint8_t data) {}
    uint8_t read() { return 0; }
    void requestFrom(uint8_t addr, uint8_t count) {}
};

extern TwoWire Wire;
