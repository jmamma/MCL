#include "EmptyTrack.h"

void EmptyTrack::clear() {
  memset(this->data, 0, EMPTY_TRACK_LEN);
}

