/* Copyright (c) 2009 - http://ruinwesen.com/ */

#pragma once

#include <inttypes.h>
#include "hardware.h"
#include "platform.h"

#ifndef ENCODER_RES_MULTIPLIER
#define ENCODER_RES_MULTIPLIER 1
#endif

#define ENCODER_FAST_SPEED 7
#define ENCODER_SLOW_SPEED 4
class EncoderParent;
typedef void (*encoder_handle_t)(EncoderParent *enc);

class EncoderParent {
public:
    int old, cur;
    #if ENCODER_RES_MULTIPLIER == 1
        int8_t rot_counter_up = 0;
        int8_t rot_counter_down = 0;
    #else
        int16_t rot_counter_up = 0;
        int16_t rot_counter_down = 0;
    #endif
    uint8_t rot_res = 1;
    encoder_handle_t handler;

    constexpr EncoderParent(encoder_handle_t _handler = nullptr)
        : old(0), cur(0), handler(_handler) {}
    void clear();
    void checkHandle();
    bool hasChanged();
    int getValue() { return cur; }
    int getOldValue() { return old; }
    void setValue(int value, bool handle = false);
    void displayAt(int i);
    virtual int update(encoder_t *enc) { return cur; }

#ifdef HOST_MIDIDUINO
    virtual ~EncoderParent() {}
#endif
};

class Encoder : public EncoderParent {
public:
    uint8_t fast_speed = 0;

    constexpr Encoder(const char *_name = nullptr,
                      encoder_handle_t _handler = nullptr)
        : EncoderParent(_handler) {}
    int update_rotations(encoder_t *enc);
    int update(encoder_t *enc) override;
};
