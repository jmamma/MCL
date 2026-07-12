#pragma once

#include "platform.h"

#if !defined(__AVR__)

#include "GridTrack.h"
#include "MidiSeqTrackData.h"

class DeviceTrack;
class GridLink;
class TrackLoadFadeData;

bool midi_track_type_is_storage_family(uint8_t track_type);
DeviceTrack *materialize_midi_storage_track(DeviceTrack *storage,
                                            uint8_t track_type,
                                            const GridLink &link,
                                            const MidiSeqTrackStorage &seq_data,
                                            const TrackLoadFadeData *load_fade,
                                            uint8_t tracknumber);

#endif // !defined(__AVR__)
