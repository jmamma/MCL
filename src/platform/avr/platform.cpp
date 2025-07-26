#include "platform.h"
#include "global.h"
#include "MidiUart.h"
#include "DebugBuffer.h"
#ifdef DEBUGMODE
DebugBuffer debugBuffer(&MidiUartUSB);
#endif
