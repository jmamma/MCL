#ifndef XOR_RAND_H
#define XOR_RAND_H

#include <Arduino.h>
#include <inttypes.h>

unsigned long tc_RandomGenerate_XorShift( void );
void tc_RandomSeed_XorShift( unsigned long newseed );

#endif // XOR_RAND_H
