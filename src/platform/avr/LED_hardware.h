#pragma once

#include "LED.h"
#include "platform.h"

class LEDHardware : public LED {
public:
  bool rec_active;
  LEDHardware() = default;
};
