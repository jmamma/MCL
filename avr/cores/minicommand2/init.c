#include <avr/io.h>
#include <util/delay.h>

#include "helpers.h"

void my_init_ram (void) __attribute__ ((naked))	\
    __attribute__ ((section (".init3")));

void
my_init_ram (void)
{
  SET_BIT(DDRB, PB0);
  CLEAR_BIT(PORTB, PB0);
  MCUCR |= _BV(SRE);
  //  MCUCR |= _BV(SRE);
  //  uint8_t *ptr = 0x2000;
  //  unsigned long i = 0;
  //  for (i = 0; i < 60000; i++) {
    //    ptr[i] = 0;
  //  }
}

