/* Copyright (c) 2009 - http://ruinwesen.com/ */

#include "Encoders.h"
#include "platform.h"
#include <stdlib.h>

void EncoderParent::checkHandle() {
    if (cur != old && handler != nullptr) {
        handler(this);
    }
    old = cur;
}

void EncoderParent::setValue(int value, bool handle) {
    if (handle) {
        cur = value;
        checkHandle();
    } else {
        old = cur = value;
    }
}

void EncoderParent::displayAt(int i) {
}

bool EncoderParent::hasChanged() {
    return old != cur;
}

void EncoderParent::clear() {
    old = 0;
    cur = 0;
}

int Encoder::update_rotations(encoder_t *enc) {
    uint8_t amount = abs(enc->normal);
    int inc = 0;

    while (amount > 0) {
        if (enc->normal > 0) {
            rot_counter_up += 1;
            if (rot_counter_up > rot_res * ENCODER_RES_MULTIPLIER) {
                rot_counter_up = 0;
                inc += 1;
            }
            rot_counter_down = 0;
        }
        if (enc->normal < 0) {
            rot_counter_down += 1;
            if (rot_counter_down > rot_res * ENCODER_RES_MULTIPLIER) {
                rot_counter_down = 0;
                inc -= 1;
            }
            rot_counter_up = 0;
        }
        amount--;
    }
    return inc;
}

int Encoder::update(encoder_t *enc) {
    int inc = update_rotations(enc);
    inc = enc->button ? inc * fast_speed : inc;
    cur += inc;
    return cur;
}

#if !defined(__AVR__)
int Encoder::applyLogicalSteps(int steps, bool fast) {
    if (steps == 0) {
        return cur;
    }

    const int direction = steps > 0 ? 1 : -1;
    int remainingSteps = abs(steps);
    const int rawUnitsPerStep = rot_res * ENCODER_RES_MULTIPLIER + 1;

    // update() remains the single owner of each encoder type's value
    // semantics. Feed it one complete detent at a time, splitting unusually
    // large resolutions so encoder_t::normal never overflows its int8 field.
    while (remainingSteps-- > 0) {
        int remainingRawUnits = rawUnitsPerStep;
        while (remainingRawUnits > 0) {
            const int chunk = remainingRawUnits > 127 ? 127 : remainingRawUnits;
            encoder_t logical = {};
            logical.normal = (int8_t)(direction * chunk);
            logical.button = fast ? 1 : 0;
            update(&logical);
            remainingRawUnits -= chunk;
        }
    }
    return cur;
}
#endif
