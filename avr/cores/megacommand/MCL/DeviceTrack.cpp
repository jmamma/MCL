#include "MCL.h"
#include "DeviceTrack.h"

uint16_t DeviceTrack::get_track_size() {
  uint16_t size = 0;
  switch (active) {
  case EMPTY_TRACK_TYPE:
    size = sizeof(GridTrack);
    break;
  case MD_TRACK_TYPE:
    size = sizeof(MDTrack);
    break;
  case A4_TRACK_TYPE:
    size = sizeof(A4Track);
    break;
  case EXT_TRACK_TYPE:
    size = sizeof(ExtTrack);
    break;
  case A4_TRACK_TYPE_270:
    size = sizeof(A4Track_270);
    break;
  case MD_TRACK_TYPE_270:
    size = sizeof(MDTrack_270);
    break;
  case EXT_TRACK_TYPE_270:
    size = sizeof(ExtTrack_270);
    break;
  }
  return size - 2;
}
