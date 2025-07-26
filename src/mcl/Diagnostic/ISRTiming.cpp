#include "ISRTiming.h"
// Initialize static members


uint32_t ISRTiming::min_duration = UINT32_MAX;
uint32_t ISRTiming::max_duration = 0;
uint32_t ISRTiming::last_duration = 0;
uint64_t ISRTiming::total_duration = 0;
uint32_t ISRTiming::start_time = 0;
uint16_t ISRTiming::last_print_time = 0;
uint16_t ISRTiming::last_reset_time = 0;
uint32_t ISRTiming::isr_count = 0;
