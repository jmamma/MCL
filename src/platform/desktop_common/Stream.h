// Stream.h — desktop shim. Mirrors Arduino's Stream base class. MidiUartClass
// derives from MidiUartParent which in turn typically expects a Stream-like
// interface; provide it here for any code paths that hold a Stream*.
#pragma once
#include "Print.h"

class Stream : public Print {
public:
    virtual int available() = 0;
    virtual int read() = 0;
    virtual int peek() = 0;

    void setTimeout(unsigned long /*ms*/) {}
};
