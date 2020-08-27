#pragma once

#include "Arduino.h"

class Stopwatch {
public:

  unsigned long time_start;

  Stopwatch() {
    time_start = micros();
  }

  unsigned long elapsed() {
    auto time_end = micros();
    return time_end - time_start;
  }
};

