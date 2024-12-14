// Add to platform.h
#pragma once
#include "platform.h"
#include "hardware/structs/psm.h"
#include "hardware/regs/m0plus.h"
#include <Arduino.h>

class StackMonitor {
private:
    static constexpr uint32_t STACK_SIZE = 4096;  // 4K per core
    static constexpr uint32_t STACK_WARN_THRESHOLD = 1024;  // Warning at 1K left

    // Get current stack pointer using inline assembly
    static inline uint32_t __attribute__((always_inline)) get_sp() {
        uint32_t sp;
        asm volatile ("mov %0, sp" : "=r"(sp));
        return sp;
    }

    // Get core-specific stack boundaries

    static void get_stack_boundaries(uint32_t& bottom, uint32_t& top) {
        // Core 0 uses SCRATCH_Y
        const uint32_t CORE0_STACK_BOTTOM = 0x20041000;  // SCRATCH_Y start
        const uint32_t CORE0_STACK_TOP    = 0x20042000;  // SCRATCH_Y end
        // Core 1 uses SCRATCH_X
        const uint32_t CORE1_STACK_BOTTOM = 0x20040000;  // SCRATCH_X start
        const uint32_t CORE1_STACK_TOP    = 0x20041000;  // SCRATCH_X end
        if (get_core_num() == 0) {
            bottom = CORE0_STACK_BOTTOM;
            top = CORE0_STACK_TOP;
        } else {
            bottom = CORE1_STACK_BOTTOM;
            top = CORE1_STACK_TOP;
        }
    }
  public:
    // Check available stack space with overflow detection
    static int32_t check_stack_space() {
        uint32_t stack_bottom, stack_top;
        get_stack_boundaries(stack_bottom, stack_top);
        uint32_t sp = get_sp();

        // If SP is above top, we've overflowed into next region
        if (sp > stack_top) {
            return -1 * (sp - stack_top);  // Return negative number indicating overflow
        }

        // If SP is below bottom, we've underflowed
        if (sp < stack_bottom) {
            return -1 * (stack_bottom - sp);  // Return negative number indicating underflow
        }

        return (sp - stack_bottom);  // Normal case: available space
    }

    // Monitor stack with warning threshold
    static bool monitor_stack() {
        int32_t space = check_stack_space();
        if (space < (int32_t)STACK_WARN_THRESHOLD) {
            gpio_put(DEBUG_PIN, 1);  // Warning LED
             print_stack_info();
            return false;
        }
        return true;
    }

    // Enhanced debug function to print stack info with overflow detection
    static void print_stack_info() {
        int32_t space = check_stack_space();
        uint32_t stack_bottom, stack_top;
        get_stack_boundaries(stack_bottom, stack_top);

        Serial.printf("Core %d Stack: SP=0x%08lx Bottom=0x%08lx Top=0x%08lx ",
            get_core_num(),
            get_sp(),
            stack_bottom,
            stack_top);

        if (space < 0) {
            Serial.printf("OVERFLOW: %ld bytes beyond boundary!\n", -space);
        } else {
            Serial.printf("Free=%ld bytes (%ld%%)\n",
                space,
                (space * 100) / STACK_SIZE);
        }
    }
};
