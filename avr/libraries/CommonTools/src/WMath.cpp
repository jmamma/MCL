/*
  Part of the Wiring project - http://wiring.org.co
  Copyright (c) 2004-06 Hernando Barragan
  Modified 13 August 2006, David A. Mellis for Arduino - http://www.arduino.cc/
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General
  Public License along with this library; if not, write to the
  Free Software Foundation, Inc., 59 Temple Place, Suite 330,
  Boston, MA  02111-1307  USA
  
  $Id$
*/

extern "C" {
  #include "stdlib.h"
}

/**
 * \addtogroup CommonTools
 *
 * @{
 *
 * \file
 * Arduino math functions
 **/

/**
 * \addtogroup helpers_wmath Arduino math functions
 *
 * @{
 **/

/** Initialize the pseudo-random number generator with seed. **/
void randomSeed(unsigned int seed)
{
  if (seed != 0) {
    srandom(seed);
  }
}

/** Returns a random value of maximum value howbig. **/
long random(long howbig)
{
  if (howbig == 0) {
    return 0;
  }
  return random() % howbig;
}

/** Returns a random value between howsmall and howbig. **/
long random(long howsmall, long howbig)
{
  if (howsmall >= howbig) {
    return howsmall;
  }
  long diff = howbig - howsmall;
  return random(diff) + howsmall;
}

/**
 * Map the value x in the range in_min - in_max to the range out_min -
 * out_max. This doesn't care about overflows.
 **/
long map(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
