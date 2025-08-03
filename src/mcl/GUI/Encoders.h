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
    bool redisplay;
    encoder_handle_t handler;

    EncoderParent(encoder_handle_t _handler = nullptr);
    virtual void clear();
    virtual void checkHandle();
    virtual bool hasChanged();
    virtual int getValue() { return cur; }
    virtual int getOldValue() { return old; }
    virtual void setValue(int value, bool handle = false);
    virtual void displayAt(int i);

#ifdef HOST_MIDIDUINO
    virtual ~EncoderParent() {}
#endif
};

class Encoder : public EncoderParent {
public:
    uint8_t fast_speed;

    Encoder(const char *_name = nullptr, encoder_handle_t _handler = nullptr);
    int update_rotations(encoder_t *enc);
    virtual int update(encoder_t *enc);
};
