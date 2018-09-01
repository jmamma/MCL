/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SHARED_H__
#define SHARED_H__

#include "MCL.h"
#include "Math.h"

struct MusicalNotes {
  const char *notes_upper[16] = {"C ", "C#", "D ", "D#", "E ", "F",
                                 "F#", "G ", "G#", "A ", "A#", "B "};
  const char *notes_lower[16] = {"c ", "c#", "d ", "d#", "e ", "f",
                                 "f#", "g ", "g#", "a ", "a#", "b "};
};

void create_chars_mixer();

#endif /* SHARED_H__ */
