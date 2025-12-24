// MCL library stubs for desktop builds
// Provides symbols that are normally provided by the embedded platform

#include <stdint.h>
#include "Arduino.h"

// Serial stub instance
SerialClass Serial;

// Machinedrum sysex header (manufacturer ID + device ID)
// 0x00 0x20 0x3c = Elektron, 0x02 = Machinedrum, 0x00 = device ID
uint8_t machinedrum_sysex_hdr[5] = {0x00, 0x20, 0x3c, 0x02, 0x00};
