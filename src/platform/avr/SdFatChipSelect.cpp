#include "AvrHardwarePins.h"

#include <SdFatConfig.h>

void sdCsInit(SdCsPin_t pin) {
  if (pin == mcl_avr_pins::sd_cs_pin) {
    mcl_avr_pins::sd_cs_output();
    mcl_avr_pins::sd_cs_write(true);
  }
}

void sdCsWrite(SdCsPin_t pin, bool level) {
  if (pin == mcl_avr_pins::sd_cs_pin) {
    mcl_avr_pins::sd_cs_write(level);
  }
}
