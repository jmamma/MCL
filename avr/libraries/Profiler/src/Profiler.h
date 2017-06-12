#ifndef PROFILER_H__
#define PROFILER_H__

#include "WProgram.h"

#ifndef HOST_MIDIDUINO
void enableProfiling();
void disableProfiling();
void sendProfilingData();
extern Task profilingTask;
#endif

#endif /* PROFILER_H__ */
