#include "MCL_impl.h"

void EmptyTrack::clear() {
  memset(this->data, 0, EMPTY_TRACK_LEN);
}

