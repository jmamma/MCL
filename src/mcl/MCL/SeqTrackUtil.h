/* Justin Mammarella jmamma@gmail.com 2024 */

#pragma once

#include "MCLSeq.h"
#if !defined(__AVR__)
#include "MCLSysConfig.h"
#include "MidiSetup.h"
#endif
#include "../Drivers/DeviceContext.h"
#include "../Drivers/MidiDevice.h"
#include "SeqPages.h"
#include "ArpSeqTrack.h"
#ifdef EXT_TRACKS
#include "SeqExtStepTrackApi.h"
#endif

class SeqTrackUtil {
public:
  static inline bool is_md_device(const MidiDevice *device) {
#if defined(PLATFORM_TBD)
    return device &&
           device->supports_capability(MidiDeviceCapability::MdSequencerTracks);
#else
    return device && device->id == DEVICE_MD;
#endif
  }

  static inline bool use_midi_tracks_for_ext() {
#if !defined(__AVR__)
#if defined(PLATFORM_TBD)
    return mcl_cfg.grid_y_device == GRID_Y_DEVICE_TBD ||
           mcl_cfg.grid_y_device == GRID_Y_DEVICE_GENER;
#else
    return mcl_cfg.grid_y_device == GRID_Y_DEVICE_GENER;
#endif
#else
    return false;
#endif
  }

  static inline uint8_t track_count(bool is_md_device) {
#ifdef EXT_TRACKS
#if !defined(__AVR__)
    if (!is_md_device && use_midi_tracks_for_ext()) {
      return mcl_seq.num_midi_tracks;
    }
#endif
    return is_md_device ? mcl_seq.num_md_tracks : mcl_seq.num_ext_tracks;
#else
    (void)is_md_device;
    return mcl_seq.num_md_tracks;
#endif
  }

  static inline SeqTrackCond &get_track(bool is_md_device, uint8_t index) {
#ifdef EXT_TRACKS
    if (!is_md_device) {
#if !defined(__AVR__)
      if (use_midi_tracks_for_ext()) {
        return static_cast<SeqTrackCond &>(mcl_seq.midi_tracks[index]);
      }
#endif
      return static_cast<SeqTrackCond &>(mcl_seq.ext_tracks[index]);
    }
#endif
    return static_cast<SeqTrackCond &>(mcl_seq.md_tracks[index]);
  }

#ifdef EXT_TRACKS
  static inline SeqExtStepTrackApi get_ext_step_track(uint8_t index) {
#if !defined(__AVR__)
    if (use_midi_tracks_for_ext()) {
      return SeqExtStepTrackApi(mcl_seq.midi_tracks[index]);
    }
#endif
    return SeqExtStepTrackApi(mcl_seq.ext_tracks[index]);
  }
#endif

#if !defined(__AVR__)
  static inline SPSXSeqTrack &get_spsx_track(uint8_t index) {
    return mcl_seq.spsx_tracks[index];
  }
#endif

  // Get base SeqTrack reference (works for both MD and SPSX)
  static inline SeqTrack &get_seq_track(bool is_md_device, uint8_t index) {
#ifdef EXT_TRACKS
    if (!is_md_device) {
#if !defined(__AVR__)
      if (use_midi_tracks_for_ext()) {
        return static_cast<SeqTrack &>(mcl_seq.midi_tracks[index]);
      }
#endif
      return static_cast<SeqTrack &>(mcl_seq.ext_tracks[index]);
    }
#endif
#if !defined(__AVR__)
    if (mcl_seq.using_spsx_tracks) {
      return static_cast<SeqTrack &>(mcl_seq.spsx_tracks[index]);
    }
#endif
    return static_cast<SeqTrack &>(mcl_seq.md_tracks[index]);
  }

  static inline ArpSeqTrack &get_arp_track(bool is_md_device, uint8_t index) {
#ifdef EXT_TRACKS
    if (!is_md_device) {
      return static_cast<ArpSeqTrack &>(mcl_seq.ext_arp_tracks[index]);
    }
#endif
    return static_cast<ArpSeqTrack &>(mcl_seq.md_arp_tracks[index]);
  }

#ifdef LFO_TRACKS
  static inline LFOSeqTrack &get_lfo_track(bool grid_x_tracks,
                                           uint8_t index) {
#ifdef EXT_TRACKS
    if (!grid_x_tracks) {
      return mcl_seq.grid_y_lfo_tracks[index];
    }
#else
    (void)grid_x_tracks;
#endif
    return mcl_seq.grid_x_lfo_tracks[index];
  }
#endif

  // ========================================================================
  // DeviceIdx-based overloads. Primary resolves the active primary backend
  // (TBD-as-primary, SPSX, or MD). Secondary resolves the active secondary
  // backend (TBD-MIDI or ext).
  // ========================================================================

  static inline uint8_t track_count(DeviceIdx device_idx) {
#if defined(PLATFORM_TBD)
    if (device_idx == DeviceIdx::Primary &&
        mcl_cfg.grid_x_device == GRID_X_DEVICE_TBD) {
      return mcl_seq.num_tbd_tracks;
    }
#endif
    return track_count(device_idx == DeviceIdx::Primary);
  }

  static inline SeqTrack &seq_track(DeviceIdx device_idx, uint8_t index) {
#if defined(PLATFORM_TBD)
    if (device_idx == DeviceIdx::Primary &&
        mcl_cfg.grid_x_device == GRID_X_DEVICE_TBD) {
      return static_cast<SeqTrack &>(mcl_seq.tbd_tracks[index]);
    }
#endif
    return get_seq_track(device_idx == DeviceIdx::Primary, index);
  }

  static inline ArpSeqTrack &arp_track(DeviceIdx device_idx, uint8_t index) {
    return get_arp_track(device_idx == DeviceIdx::Primary, index);
  }

#ifdef LFO_TRACKS
  static inline LFOSeqTrack &lfo_track(DeviceIdx device_idx, uint8_t index) {
    return get_lfo_track(device_idx == DeviceIdx::Primary, index);
  }
#endif

  template <typename Fn>
  static inline void for_each_track(bool is_md_device, Fn fn) {
    uint8_t len = track_count(is_md_device);
    for (uint8_t i = 0; i < len; ++i) {
      fn(get_track(is_md_device, i), i);
    }
  }

  // Iterate using SeqTrack base (works for both MD and SPSX)
  template <typename Fn>
  static inline void for_each_seq_track(bool is_md_device, Fn fn) {
    uint8_t len = track_count(is_md_device);
    for (uint8_t i = 0; i < len; ++i) {
      fn(get_seq_track(is_md_device, i), i);
    }
  }

  // ========================================================================
  // Template dispatch: call fn with the correct concrete MD track type.
  // On AVR the SPSX branch is compiled out — zero overhead.
  // ========================================================================

  // Single track by index
  template <typename Fn>
  static inline void with_md_track(uint8_t n, Fn fn) {
#if !defined(__AVR__)
    if (mcl_seq.using_spsx_tracks) {
      fn(mcl_seq.spsx_tracks[n]);
    } else
#endif
    {
      fn(mcl_seq.md_tracks[n]);
    }
  }

  // All MD tracks
  template <typename Fn>
  static inline void for_each_md_track(Fn fn) {
    uint8_t len = mcl_seq.num_md_tracks;
#if !defined(__AVR__)
    if (mcl_seq.using_spsx_tracks) {
      for (uint8_t i = 0; i < len; i++) fn(mcl_seq.spsx_tracks[i], i);
    } else
#endif
    {
      for (uint8_t i = 0; i < len; i++) fn(mcl_seq.md_tracks[i], i);
    }
  }

  // Single track, return a value
  template <typename Fn>
  static inline auto with_md_track_r(uint8_t n, Fn fn)
      -> decltype(fn(mcl_seq.md_tracks[0])) {
#if !defined(__AVR__)
    if (mcl_seq.using_spsx_tracks) {
      return fn(mcl_seq.spsx_tracks[n]);
    }
#endif
    return fn(mcl_seq.md_tracks[n]);
  }
};
