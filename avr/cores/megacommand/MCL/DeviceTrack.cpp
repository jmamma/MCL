#include "DeviceTrack.h"
#include "MCL.h"
#include "new.h"

bool DeviceTrack::init_track_type(uint8_t track_type) {
  switch (track_type) {
  case A4_TRACK_TYPE_270:
  case MD_TRACK_TYPE_270:
  case EXT_TRACK_TYPE_270:
/*    if (!data) {
      // no space for track upgrade
      return false;
    } else {
      // TODO upgrade right here
      return true;
    } */
    return false;
    break;
  case EMPTY_TRACK_TYPE:
    ::new(this) EmptyTrack;
    break;
  case MD_TRACK_TYPE:
    ::new(this) MDTrack;
    break;
  case A4_TRACK_TYPE:
    ::new(this) A4Track;
    break;
  case EXT_TRACK_TYPE:
    ::new(this) ExtTrack;
    break;
  }
  return true;
}
