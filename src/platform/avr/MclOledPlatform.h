#pragma once

#include "AvrHardwarePins.h"

inline void mcl_oled_pins_output() { mcl_avr_pins::oled_pins_output(); }
inline void mcl_oled_cs_high() { mcl_avr_pins::oled_cs_high(); }
inline void mcl_oled_cs_low() { mcl_avr_pins::oled_cs_low(); }
inline void mcl_oled_dc_high() { mcl_avr_pins::oled_dc_high(); }
inline void mcl_oled_dc_low() { mcl_avr_pins::oled_dc_low(); }
inline void mcl_oled_reset_high() { mcl_avr_pins::oled_reset_high(); }
inline void mcl_oled_reset_low() { mcl_avr_pins::oled_reset_low(); }

void mcl_oled_spi_acquire();
void mcl_oled_spi_release();
