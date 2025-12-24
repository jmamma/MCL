#pragma once

// MidiUart.h stub for desktop builds
// This provides a minimal stub that works with the real MidiUartParent.h

#include <stdint.h>

// Forward declarations - the real classes are defined in the MCL sources
class MidiClass;
class MidiUartParent;

// Desktop stub for MidiUartClass
// Note: On embedded, this inherits from MidiUartParent, but for desktop
// we provide a standalone stub with all needed methods
class MidiUartClass {
public:
    MidiClass* midi = nullptr;
    uint8_t device = 0;

    MidiUartClass() {}

    void initSerial() {}
    void sendSysex(uint8_t* data, uint16_t len) { (void)data; (void)len; }
    void putc(uint8_t c) { (void)c; }
    void m_putc(uint8_t c) { (void)c; }
    void m_putc(uint8_t* data, uint8_t len) { (void)data; (void)len; }
    void puts(const uint8_t* data, uint16_t len) { (void)data; (void)len; }
    void sendRaw(uint8_t b) { (void)b; }
    void sendNoteOff(uint8_t ch, uint8_t note) { (void)ch; (void)note; }
    void sendNoteOff(uint8_t ch, uint8_t note, uint8_t vel) { (void)ch; (void)note; (void)vel; }
    void sendNoteOn(uint8_t ch, uint8_t note, uint8_t vel) { (void)ch; (void)note; (void)vel; }
    void sendCC(uint8_t ch, uint8_t cc, uint8_t val) { (void)ch; (void)cc; (void)val; }
    void sendPolyKeyPressure(uint8_t ch, uint8_t note, uint8_t pressure) { (void)ch; (void)note; (void)pressure; }
    void sendProgramChange(uint8_t ch, uint8_t program) { (void)ch; (void)program; }
    void sendChannelPressure(uint8_t ch, uint8_t pressure) { (void)ch; (void)pressure; }
    void sendPitchBend(uint8_t ch, int16_t bend) { (void)ch; (void)bend; }

    // Methods that might be called from real code
    bool avail() { return false; }
    uint8_t getc() { return 0; }
};

inline MidiUartClass MidiUart;
inline MidiUartClass MidiUart2;
