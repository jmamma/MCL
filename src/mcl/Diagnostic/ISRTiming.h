#pragma once
#include "platform.h"
#include "global.h"

class ISRTiming {
private:
    static uint32_t min_duration;
    static uint32_t max_duration;
    static uint32_t last_duration;
    static uint64_t total_duration;
    static uint32_t start_time;
    static uint16_t last_print_time;
    static uint16_t last_reset_time;
    static uint32_t isr_count;

public:
    static void init() {
        max_duration = 0;
        min_duration = UINT32_MAX;  // Initialize to max possible value
        last_duration = 0;
        total_duration = 0;
        isr_count = 0;
        last_print_time = read_clock_ms();
        last_reset_time = read_clock_ms();
    }

    static inline void enter_isr() {
        start_time = time_us_32();
    }

    static inline void exit_isr() {
        uint32_t duration = time_us_32() - start_time;
        
        if (duration > max_duration) {
            max_duration = duration;
        }
        if (duration < min_duration) {
            min_duration = duration;
        }
        last_duration = duration;
        total_duration += duration;
        isr_count++;
    }

    static void print_stats() {
        // Don't print if we haven't seen any ISRs yet
        if (isr_count == 0) {
            return;
        }

        uint16_t current_time = read_clock_ms();
        
        // Auto clear stats after 10 seconds
        if (clock_diff(last_reset_time, current_time) >= 10000) {
            reset_stats();
            last_reset_time = current_time;
            return;
        }
        
        // Only print if more than 1 second has elapsed
        if (clock_diff(last_print_time, current_time) < 1000) {
            return;
        }
        
        DEBUG_PRINT("ISR Statistics:");
        DEBUG_PRINTLN("");
        DEBUG_PRINT("Min duration: ");
        DEBUG_PRINT((uint32_t)min_duration);
        DEBUG_PRINT(" us");
        DEBUG_PRINTLN("");
        DEBUG_PRINT("Max duration: ");
        DEBUG_PRINT((uint32_t)max_duration);
        DEBUG_PRINT(" us");
        DEBUG_PRINTLN("");
        DEBUG_PRINT("Last duration: ");
        DEBUG_PRINT((uint32_t)last_duration);
        DEBUG_PRINT(" us");
        DEBUG_PRINTLN("");
        DEBUG_PRINT("Average duration: ");
        DEBUG_PRINT((uint32_t)(isr_count ? (total_duration / isr_count) : 0));
        DEBUG_PRINT(" us");
        DEBUG_PRINTLN("");
        DEBUG_PRINT("Total ISR calls: ");
        DEBUG_PRINT((uint32_t)isr_count);
        DEBUG_PRINTLN("");
        last_print_time = current_time;
    }

    static void reset_stats() {
        max_duration = 0;
        min_duration = UINT32_MAX;  // Reset to max possible value
        last_duration = 0;
        total_duration = 0;
        isr_count = 0;
    }
};
extern ISRTiming isrTiming;
