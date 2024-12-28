#pragma once
#include <Arduino.h>

class StackMonitor {
private:
    static constexpr uint32_t STACK_WARN_THRESHOLD = 1024;  // Warning at 1K left

    // These match the linker script exactly
    static constexpr uint32_t SCRATCH_X_START = 0x20080000;
    static constexpr uint32_t SCRATCH_Y_START = 0x20081000;
    static constexpr uint32_t SCRATCH_SIZE = 4096;  // 4K per scratch space

public:
    static bool monitor_stack() {
        int32_t free_space = get_free_stack();
        if (free_space < STACK_WARN_THRESHOLD) {
            print_stack_info();
            return false;
        }
        return true;
    }

    static int32_t get_free_stack() {
        uint32_t sp = rp2040.getStackPointer();
        uint32_t bottom, top;

        extern bool core1_separate_stack;
        extern uint32_t* core1_separate_stack_address;

        if (rp2040.cpuid() == 0) {
            // Core 0 uses SCRATCH_Y
            bottom = SCRATCH_Y_START;
            top = bottom + SCRATCH_SIZE;
        } else {
            if (core1_separate_stack && core1_separate_stack_address) {
                // Using separate allocated stack
                bottom = (uint32_t)core1_separate_stack_address;
                top = bottom + SCRATCH_SIZE * 2;  // 8KB when separate
            } else {
                // Core 1 uses SCRATCH_X
                bottom = SCRATCH_X_START;
                top = bottom + SCRATCH_SIZE;
            }
        }

        if (sp > top) {
            return -(sp - top);  // Stack overflow
        }
        if (sp < bottom) {
            return -(bottom - sp);  // Stack underflow
        }
        
        return sp - bottom;  // Available space
    }

    static void print_stack_info() {
        int32_t free_space = get_free_stack();
        uint32_t core = rp2040.cpuid();
        uint32_t sp = rp2040.getStackPointer();
        
        char buf[128];
        if (free_space < 0) {
            snprintf(buf, sizeof(buf), 
                    "Core %lu Stack: SP=0x%08lx OVERFLOW: %ld bytes beyond boundary",
                    core, sp, -free_space);
        } else {
            uint32_t stack_size = (core == 0 || 
                                (core1_separate_stack && core1_separate_stack_address)) 
                                ? SCRATCH_SIZE : SCRATCH_SIZE;
            snprintf(buf, sizeof(buf), 
                    "Core %lu Stack: SP=0x%08lx Free=%ld bytes (%ld%%)",
                    core, sp, free_space, (free_space * 100) / stack_size);
        }
        DEBUG_PRINTLN(buf);
    }
};
