#ifndef CORE_H__
#define CORE_H__

#define MEGACOMMAND

#if defined(MEGACOMMAND) && defined(IS_ISR_ROUTINE)
  #define ALWAYS_INLINE() __attribute__((always_inline))
  #define FORCED_INLINE() __attribute__((always_inline))
#elif defined(MEGACOMMAND)
  #define ALWAYS_INLINE()
  #define FORCED_INLINE() __attribute__((always_inline))
#else
  #define ALWAYS_INLINE()
  #define FORCED_INLINE()
#endif

#endif /* CORE_H__ */
