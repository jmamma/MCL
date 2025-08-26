#include "platform.h"
#include "DebugBuffer.h"
#include "ISRTiming.h"
volatile uint32_t interrupt_lock_count = 0;
#ifdef DEBUGMODE
DebugBuffer debugBuffer(&Serial);
#endif
ISRTiming isrTiming;

#ifdef MULTICORE
bool core1_separate_stack = true;
#endif
