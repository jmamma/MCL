// LED_hardware.h — desktop platform LED stub. Mirrors the non-TBD branch of
// MCL/src/platform/rp2040/LED_hardware.h: LEDHardware simply inherits MCL's
// own LED class (src/mcl/GUI/LED.h) whose methods are already no-ops on
// platforms without addressable LEDs.
#pragma once

#define LED_BLINK_PERIOD   1000
#define LED_FLASH_DURATION 50

#include "LED.h"
#include "platform.h"

#define COLOR(r, g, b) (uint32_t)(((r) << 16) | ((g) << 8) | (b))
#define STRIP_RED      COLOR(255, 0, 0)
#define STRIP_WHITE    COLOR(255, 255, 255)
#define STRIP_BLACK    COLOR(0, 0, 0)
#define STRIP_LED1 16
#define STRIP_LED2 17
#define STRIP_LED3 18

class LEDHardware : public LED {
public:
    LEDHardware() : LED() {}

    // SeqPage references LEDHardware::rec_active to drive a recording-status
    // LED. Desktop has no LED; carry the flag so the field exists.
    bool rec_active = false;
};
