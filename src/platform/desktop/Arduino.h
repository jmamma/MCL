#pragma once

// Arduino.h stub for desktop builds

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

// Arduino types
typedef uint8_t byte;
typedef bool boolean;

// Arduino constants
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2

// Arduino functions (stubs)
#define pinMode(pin, mode)
#define digitalWrite(pin, val)
#define digitalRead(pin) 0
#define analogRead(pin) 0
#define analogWrite(pin, val)
#define delay(ms)
#define delayMicroseconds(us)
#define millis() 0UL
#define micros() 0UL

// String class stub
class String {
public:
    String() {}
    String(const char* s) {}
    String(int n) {}
    const char* c_str() const { return ""; }
    int length() const { return 0; }
};

// Serial stub
class SerialClass {
public:
    void begin(long baud) {}
    void print(const char* s) {}
    void print(int n) {}
    void println(const char* s) {}
    void println(int n) {}
    void println() {}
    int available() { return 0; }
    int read() { return -1; }
    void write(uint8_t b) {}
};

extern SerialClass Serial;

// Memory functions
#define F(x) (x)
