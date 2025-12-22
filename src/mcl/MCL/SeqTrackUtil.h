/* Justin Mammarella jmamma@gmail.com 2024 */

#pragma once

#include "MCLSeq.h"
#include "MD.h"
#include "SeqPages.h"
#include "ArpSeqTrack.h"

class SeqTrackUtil {
public:
  static inline bool is_md_device(const MidiDevice *device) {
    return device == &MD;
  }

  static inline uint8_t track_count(bool is_md_device) {
#ifdef EXT_TRACKS
    return is_md_device ? mcl_seq.num_md_tracks : mcl_seq.num_ext_tracks;
#else
    (void)is_md_device;
    return mcl_seq.num_md_tracks;
#endif
  }

  static inline SeqTrackCond &get_track(bool is_md_device, uint8_t index) {
#ifdef EXT_TRACKS
    if (!is_md_device) {
      return static_cast<SeqTrackCond &>(mcl_seq.ext_tracks[index]);
    }
#endif
    return static_cast<SeqTrackCond &>(mcl_seq.md_tracks[index]);
  }

  static inline ArpSeqTrack &get_arp_track(bool is_md_device, uint8_t index) {
#ifdef EXT_TRACKS
    if (!is_md_device) {
      return static_cast<ArpSeqTrack &>(mcl_seq.ext_arp_tracks[index]);
    }
#endif
    return static_cast<ArpSeqTrack &>(mcl_seq.md_arp_tracks[index]);
  }

  static inline void sync_ext_length_encoder(bool is_md_device,
                                             uint8_t track_idx,
                                             uint8_t length, bool force) {
#ifdef EXT_TRACKS
    if (!is_md_device && (force || last_ext_track == track_idx)) {
      seq_extparam4.cur = length;
    }
#else
    (void)is_md_device;
    (void)track_idx;
    (void)length;
    (void)force;
#endif
  }

  template <typename Fn>
  static inline void for_each_track(bool is_md_device, Fn fn) {
    uint8_t len = track_count(is_md_device);
    for (uint8_t i = 0; i < len; ++i) {
      fn(get_track(is_md_device, i), i);
    }
  }
};
