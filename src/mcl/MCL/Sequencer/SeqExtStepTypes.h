/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQEXTSTEPTYPES_H__
#define SEQEXTSTEPTYPES_H__

#include <stdint.h>

#if defined(__AVR__)
using seq_extstep_tick_t = int16_t;
#else
using seq_extstep_tick_t = int32_t;
#endif

#endif /* SEQEXTSTEPTYPES_H__ */
