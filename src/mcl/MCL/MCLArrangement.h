/**
 * MCLArrangement - project-local arrangement storage.
 */
#pragma once

#include "MCLPlatformFeatures.h"

#if MCL_FEATURE_HOST_ARRANGER

#include "MCLArrangementFormat.h"
#include "MCLMemory.h"
#include "TrackLoadFade.h"
#include <stdint.h>

class DeviceTrack;
class EmptyTrack;

class MCLArrangement {
public:
  bool ensureActive();
  bool readMeta(mclarrfile::Header *header);
  bool clearActive();
  bool saveActive();
  bool select(uint8_t idx);
  bool create(uint8_t idx, const char *name);
  bool createFirst(uint8_t *idxOut);
  bool importGrid(uint16_t trackMask, uint8_t startRow);
  void tick();
  void resetPlayback();
  void resetPlaybackForTransport();
  void setLoopRegion(uint32_t startQ12, uint32_t endQ12);
  void clearLoopRegion();
  bool seekLoad(uint32_t positionQ12, bool immediate,
                bool allowPrestartFade = false);
  bool armRuntimeForHostLoad(uint32_t positionQ12,
                             const GridRow rows[NUM_SLOTS],
                             uint16_t trackMask, GridSlot loadOffset,
                             GridIndex sourceGridBank = 0,
                             const uint32_t privateSourceIds[NUM_SLOTS] =
                                 nullptr);
  bool releasePlaybackTracks(uint32_t trackMask);
  uint32_t playbackReleasedMask() const { return playback_released_mask_; }
  void armRuntimeFade(uint8_t dst, const TrackLoadFadeData &fade);
  bool applyClipRuntime(uint8_t dst, DeviceTrack *track);

  uint16_t readClips(uint32_t startQ12, uint32_t endQ12, uint16_t skip,
                     uint8_t maxClips, mclarrfile::Clip *out,
                     uint32_t *totalMatches = nullptr,
                     bool *more = nullptr);
  uint16_t readMarkers(uint32_t startQ12, uint32_t endQ12, uint16_t skip,
                       uint8_t maxMarkers, mclarrfile::Marker *out,
                       uint32_t *totalMatches = nullptr,
                       bool *more = nullptr);
  uint16_t readLoopRegions(uint32_t startQ12, uint32_t endQ12, uint16_t skip,
                           uint8_t maxLoopRegions,
                           mclarrfile::LoopRegion *out,
                           uint32_t *totalMatches = nullptr,
                           bool *more = nullptr);
  bool readTrackLabels(
      char labels[mclarrfile::kTrackLabelCount][mclarrfile::kTrackLabelBytes]);
  bool setTrackLabel(uint8_t track, const char *label);
  bool setMarkerLabel(uint32_t startQ12, uint8_t track, const char *label);
  bool setLoopRegionRecord(const mclarrfile::LoopRegion &region);
  bool setClipFade(uint32_t startQ12, uint32_t durationQ12, uint8_t track,
                   uint8_t row, bool fadeOut, bool overrideFade,
                   const TrackLoadFadeData &fade);
  bool makeClipLocal(uint32_t startQ12, uint32_t durationQ12, uint8_t track,
                     uint8_t row, GridSlot expectedSourceSlot);
  bool exportPrivateSourceToGrid(uint32_t sourceId, GridSlot sourceSlot,
                                 GridRow sourceRow, GridSlot targetSlot,
                                 GridRow targetRow);
  void beginQueuedPrivateLoads(const uint32_t sourceIds[NUM_SLOTS]);
  void endQueuedPrivateLoads();
  bool loadQueuedPrivateSource(GridSlot sourceSlot, GridRow row,
                               EmptyTrack &scratch, DeviceTrack **out);

private:
  bool openActive(class FsFile *file, uint8_t mode);
  bool openIndex(class FsFile *file, uint8_t idx, uint8_t mode);
  void armClipRuntime(uint8_t dst, const mclarrfile::Clip &clip,
                      uint16_t elapsedQ12 = 0);
  bool queueClipStarts(uint32_t startQ12, uint32_t endQ12,
                       bool loadActiveAtPosition,
                       bool clearInactiveTracks,
                       uint8_t loadQueueFlags);
  bool rewriteActive(const mclarrfile::Header &header,
                     const mclarrfile::Clip *clips, uint32_t clipCount);
  bool rewriteActiveWithMarkers(const mclarrfile::Header &header,
                                const mclarrfile::Clip *clips,
                                uint32_t clipCount,
                                const mclarrfile::Marker *markers,
                                uint16_t markerCount);
  bool rewriteActiveWithMetadata(
      const mclarrfile::Header &header, const mclarrfile::Clip *clips,
      uint32_t clipCount, const mclarrfile::Marker *markers,
      uint16_t markerCount,
      const char trackLabels[mclarrfile::kTrackLabelCount]
                            [mclarrfile::kTrackLabelBytes],
      const mclarrfile::LoopRegion *loopRegions = nullptr,
      uint16_t loopRegionCount = 0);

  uint8_t playback_arrangement_idx_ = 0xFF;
  uint32_t last_tick_q12_ = 0;
  uint32_t playback_active_mask_ = 0;
  uint32_t playback_released_mask_ = 0;
  static const uint8_t kRuntimeSlots = 16;
  TrackLoadFadeData clip_runtime_fades_[kRuntimeSlots];
  uint32_t clip_runtime_fade_mask_ = 0;
  uint32_t queued_private_source_ids_[NUM_SLOTS] = {};
  bool playback_active_ = false;
  bool loop_enabled_ = false;
  bool loop_entered_ = false;
  uint32_t loop_start_q12_ = 0;
  uint32_t loop_end_q12_ = 0;
  uint16_t stored_loop_active_id_ = 0;
  uint16_t stored_loop_repeats_done_ = 0;
  uint32_t stored_loop_start_q12_ = 0;
  uint32_t stored_loop_end_q12_ = 0;
  uint16_t stored_loop_count_ = 0;
};

extern MCLArrangement mcl_arrangement;

#endif // MCL_FEATURE_HOST_ARRANGER
