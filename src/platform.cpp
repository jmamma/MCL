#include "platform.h"
#include "DebugBuffer.h"

volatile uint32_t interrupt_lock_count = 0;
DebugBuffer debugBuffer;
