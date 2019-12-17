#ifndef CORE_H__
#define CORE_H__

#define MEGACOMMAND

#ifdef MEGACOMMAND
 // #define ALWAYS_INLINE()
  #define ALWAYS_INLINE() __attribute__((always_inline))
#else
  #define ALWAYS_INLINE()
#endif

#endif /* CORE_H__ */
