// hardware.cpp — desktop GPIO mock + Wire/SPI/Serial singletons.
//
// digitalWrite/digitalRead/pinMode go through desktop_gpio_state[], so JUCE
// host code can prod pin state from outside if it ever needs to simulate
// encoder/button input. pinMode is a no-op (we don't track direction).
#include "Arduino.h"
#include "hardware.h"
#include "Wire.h"
#include "SPI.h"
#include "HardwareSerial.h"
#include "DebugBuffer.h"

extern "C" {
uint8_t desktop_gpio_state[256] = {0};
}

void pinMode(uint8_t /*pin*/, uint8_t /*mode*/) {
    // Desktop doesn't track pin direction.
}

void digitalWrite(uint8_t pin, uint8_t val) {
    desktop_gpio_state[pin] = val ? 1 : 0;
}

int digitalRead(uint8_t pin) {
    return desktop_gpio_state[pin];
}

// Global instances referenced by extern in the headers.
HardwareSerial Serial;
HardwareSerial Serial1;
HardwareSerial Serial2;
TwoWire        Wire;
SPIClass       SPI;
SPIClass       SPI1;
DebugBuffer    debugBuffer(&Serial);

// Hardware noop stubs declared in hardware.h. picow_init is the rp2040 WiFi
// chip init — irrelevant on desktop. change_usb_mode is the USB DFU/storage
// mode switch — also irrelevant.
void change_usb_mode(uint8_t /*mode*/) {}
void picow_init() {}
