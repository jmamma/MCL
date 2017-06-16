#ifndef IOXXX_CPP_MACROS_H__
#define IOXXX_CPP_MACROS_H__

#ifndef __SFR_OFFSET_CXX
/* Define as 0 before including this file for compatibility with old asm
   sources that don't subtract __SFR_OFFSET from symbolic I/O addresses.  */
#  if __AVR_ARCH__ >= 100
#    define __SFR_OFFSET_CXX 0x00
#  else
#    define __SFR_OFFSET_CXX 0x20
#  endif
#endif

/*

 *** template class example ***

#include "iom64cxx.h"

template <uint16_t REG> class RegTest {
public:
  RegTest() {
    CXX_IO8(REG) = 0;
  }
};

RegTest<CXX_PORTA> regTest;

*/

/* macros to access the registers later on in the c++ class */
#define CXX_IO8(x) (*(volatile uint8_t *)(x))
#define CXX_IO16(x) (*(volatile uint16_t *)(x))

/* macros that will work as template arguments */
#define _SFR_MEM8_CXX(mem_addr) (mem_addr)
#define _SFR_MEM16_CXX_CXX(mem_addr) (mem_addr)
#define _SFR_MEM32_CXX(mem_addr) (mem_addr)
#define _SFR_IO8_CXX(io_addr) ((io_addr) + __SFR_OFFSET_CXX)
#define _SFR_IO16_CXX_CXX(io_addr) ((io_addr) + __SFR_OFFSET_CXX)

#define _SFR_IO_ADDR_CXX(sfr) ((sfr) - __SFR_OFFSET_CXX)
#define _SFR_MEM_ADDR_CXX(sfr) (sfr)
#define _SFR_IO_REG_P_CXX(sfr) ((sfr) < 0x40 + __SFR_OFFSET_CXX)

#if (__SFR_OFFSET == 0x20)
/* No need to use ?: operator, so works in assembler too.  */
#define _SFR_ADDR(sfr) _SFR_MEM_ADDR(sfr)
#elif !defined(__ASSEMBLER__)
#define _SFR_ADDR(sfr) (_SFR_IO_REG_P(sfr) ? (_SFR_IO_ADDR(sfr) + 0x20) : _SFR_MEM_ADDR(sfr))
#endif

#endif /* IOXXX_CPP_MACROS_H__ */

