#include "MCL.h"
#include "DeviceTrack.h"

uint16_t DeviceTrack::get_track_size() {
  switch (active) {
  case EMPTY_TRACK_TYPE:
    return sizeof(GridTrack);
    break;
  case MD_TRACK_TYPE:
    return sizeof(MDTrack);
    break;
  case A4_TRACK_TYPE:
    return sizeof(A4Track);
    break;
  case EXT_TRACK_TYPE:
    return sizeof(ExtTrack);
    break;
  case A4_TRACK_TYPE_270:
    return sizeof(A4Track_270);
    break;
  case MD_TRACK_TYPE_270:
    return sizeof(MDTrack_270);
    break;
  case EXT_TRACK_TYPE_270:
    return sizeof(ExtTrack_270);
    break;
  }
  return 0;
}
