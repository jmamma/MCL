/* Copyright (c) 2009 - http://ruinwesen.com/ */

#pragma once

#include <inttypes.h>
#include "hardware.h"

class EncoderParent;
typedef void (*encoder_handle_t)(EncoderParent *enc);

class EncoderParent {
public:
    int old, cur;
    int8_t rot_counter_up = 0;
    int8_t rot_counter_down = 0;
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
    bool fastmode;

    Encoder(const char *_name = nullptr, encoder_handle_t _handler = nullptr);
    int update_rotations(encoder_t *enc);
    virtual int update(encoder_t *enc);
};
