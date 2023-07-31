/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SHARED_H__
#define SHARED_H__

#include "mcl.h"
#include "math.h"

struct MusicalNotes {
  const char *notes_upper[16] = {"C ", "C#", "D ", "D#", "E ", "F ",
                                 "F#", "G ", "G#", "A ", "A#", "B "};
  const char *notes_lower[16] = {"c ", "c#", "d ", "d#", "e ", "f ",
                                 "f#", "g ", "g#", "a ", "a#", "b "};
};

inline int range_limit(int val, int min, int max) {
  if (val > max) { return max; }
  if (val < min) { return min; }
  return val;
}

inline bool range_check(int val, int min, int max) {
  if (val > max) { return false; }
  if (val < min) { return false; }
  return true;
}

#endif /* SHARED_H__ */
