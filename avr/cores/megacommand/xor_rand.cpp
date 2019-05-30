#include "xor_rand.h"
// taken from: https://code.google.com/archive/p/arduino-tiny/source/core2/source

// http://www.jstatsoft.org/v08/i14/paper
// ...Marsaglia's favourite

static uint32_t Seed = 1;

unsigned long tc_RandomGenerate_XorShift( void )
{
  uint32_t Y;
  
  Y = Seed;
  
  Y = Y ^ (Y * 8192L);
  Y = Y ^ (Y / 131072L);
  Y = Y ^ (Y * 32L);

  Seed = Y;
  
  return( Y );
}

void tc_RandomSeed_XorShift( unsigned long newseed )
{
  if ( newseed != 0 )
    Seed = newseed;
}

