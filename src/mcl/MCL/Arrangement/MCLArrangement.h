/**
 * MCLArrangement - project-local arrangement storage.
 */
#pragma once

#include "MCLPlatformFeatures.h"

#if MCL_FEATURE_HOST_ARRANGER

#include "Arrangement/MCLArrangementFormat.h"
#include "MCLMemory.h"
#include "Grid/TrackLoadFade.h"
#include <stdint.h>

class DeviceTrack;
class EmptyTrack;

class MCLArrangement {
public:
  struct AutomationRuntimeLane {
    bool active = false;
    bool have_prev = false;
    bool have_next = false;
    bool last_sent_valid = false;
    uint32_t next_point_index = 0;
    mclarrfile::AutomationPoint prev = {};
    mclarrfile::AutomationPoint next = {};
    uint16_t last_sent_value = 0;
  };

  struct AutomationPendingWrite {
    uint8_t track = 0;
    uint8_t targetType = 0;
    uint8_t device = 0;
    uint8_t targetIndex = 0;
    uint8_t valueType = 0;
    uint16_t value = 0;
  };

  struct PrivateSourcePreviewNote {
    uint8_t start = 0;
    uint8_t length = 1;
    uint8_t note = 0;
    uint8_t velocity = 0;
    int8_t timing = 0;
  };

  bool ensureActive();
  bool readMeta(mclarrfile::Header *header);
  bool clearActive();
  bool saveActive();
  bool select(uint8_t idx);
  bool create(uint8_t idx, const char *name);
  bool createFirst(uint8_t *idxOut);
  bool importGrid(uint16_t trackMask, uint8_t startRow);
  bool replaceClips(const mclarrfile::Clip *clips, uint32_t clipCount);
  void tick();
  void resetPlayback(bool clearPrivateSources = true);
  void resetPlaybackForTransport(bool clearReleasedTracks = true);
  void reconcilePlaybackAfterEdit(bool clearReleasedTracks = true);
  void setHostPlaybackSuspended(bool suspended);
  void setLoopRegion(uint32_t startQ12, uint32_t endQ12);
  void clearLoopRegion();
  bool seekLoad(uint32_t positionQ12, bool immediate,
                bool allowPrestartFade = false,
                bool clearReleasedTracks = true);
  bool seekLoadCurrentPosition(bool immediate,
                               bool allowPrestartFade = false,
                               bool clearReleasedTracks = true);
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
  void flushAutomationWrites();
  uint32_t runtimePrivateSourceMask() const;
  uint32_t runtimePrivateSourceId(uint8_t dst) const;
  void setRuntimePrivateSource(uint8_t dst, uint32_t sourceId,
                               uint8_t sourceSlot);
  void clearRuntimePrivateSource(uint8_t dst);
  void clearRuntimePrivateSources(uint32_t mask);
  bool flushRuntimePrivateSource(uint8_t dst);
  bool flushRuntimePrivateSourceEdits();
  bool markRuntimePrivateSourceEdited(uint8_t dst);

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
  uint16_t readAutomationLanes(uint16_t skip, uint8_t maxLanes,
                               mclarrfile::AutomationLane *out,
                               uint32_t *totalMatches = nullptr,
                               bool *more = nullptr);
  uint16_t readAutomationPoints(uint32_t pointOffset, uint16_t pointCount,
                                uint16_t skip, uint8_t maxPoints,
                                mclarrfile::AutomationPoint *out,
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
  bool setAutomationLanePoints(const mclarrfile::AutomationLane &lane,
                               const mclarrfile::AutomationPoint *points,
                               uint16_t pointCount);
  bool makeClipLocal(uint32_t startQ12, uint32_t durationQ12, uint8_t track,
                     uint8_t row, GridSlot expectedSourceSlot,
                     uint32_t *sourceIdOut = nullptr);
  bool createPrivateSourceFromGrid(GridSlot sourceSlot, GridRow row,
                                   GridSlot dstTrack,
                                   uint32_t *sourceIdOut = nullptr);
  bool duplicatePrivateSource(uint32_t sourceId, GridSlot sourceSlot,
                              GridSlot dstTrack,
                              uint32_t *sourceIdOut = nullptr);
  bool privateSourcePreview(uint32_t sourceId, uint8_t *trackType,
                            uint8_t *length, uint8_t *speed,
                            uint64_t *trigMask,
                            uint8_t *noteCount = nullptr,
                            uint8_t *noteMin = nullptr,
                            uint8_t *noteMax = nullptr,
                            uint8_t *noteFlags = nullptr,
                            PrivateSourcePreviewNote *notes = nullptr,
                            uint8_t maxNotes = 0,
                            int8_t *trigTiming = nullptr,
                            uint8_t maxTrigTiming = 0);
  bool exportPrivateSourceToGrid(uint32_t sourceId, GridSlot sourceSlot,
                                 GridRow sourceRow, GridSlot targetSlot,
                                 GridRow targetRow);
  void beginQueuedPrivateLoads(const uint32_t sourceIds[NUM_SLOTS]);
  void endQueuedPrivateLoads();
  bool loadQueuedPrivateSource(GridSlot sourceSlot, GridRow row,
                               EmptyTrack &scratch, DeviceTrack **out,
                               GridSlot runtimeSlot = 255);

private:
  struct AutomationWriteData {
    const mclarrfile::AutomationLane *lanes = nullptr;
    const mclarrfile::AutomationPoint *points = nullptr;
    uint16_t lane_count = 0;
    uint32_t point_count = 0;
  };

  bool openActive(class FsFile *file, int mode);
  bool openIndex(class FsFile *file, uint8_t idx, int mode);
  void armClipRuntime(uint8_t dst, const mclarrfile::Clip &clip,
                      uint16_t elapsedQ12 = 0);
  void resetAutomationRuntime();
  bool loadAutomationDirectory();
  bool automationReadPoint(class FsFile &file, uint32_t pointIndex,
                           mclarrfile::AutomationPoint *out);
  bool automationPrepareLane(class FsFile &file, uint16_t laneIndex,
                             uint32_t positionQ12);
  uint16_t automationEvaluate(uint16_t laneIndex, uint32_t positionQ12) const;
  void queueAutomationWrite(uint16_t laneIndex, uint16_t value);
  void chaseAutomation(uint32_t positionQ12, bool sendValues);
  void tickAutomation(uint32_t positionQ12);
  bool queueClipStarts(uint32_t startQ12, uint32_t endQ12,
                       bool loadActiveAtPosition,
                       bool clearInactiveTracks,
                       uint8_t loadQueueFlags,
                       bool honorReleasedTracks);
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
      uint16_t loopRegionCount = 0,
      const AutomationWriteData *automationOverride = nullptr);

  uint8_t playback_arrangement_idx_ = 0xFF;
  uint32_t last_tick_q12_ = 0;
  uint32_t playback_active_mask_ = 0;
  uint32_t playback_released_mask_ = 0;
  static const uint8_t kRuntimeSlots = 16;
  TrackLoadFadeData clip_runtime_fades_[kRuntimeSlots];
  uint32_t clip_runtime_fade_start_q12_[kRuntimeSlots] = {};
  uint32_t clip_runtime_fade_mask_ = 0;
  static const uint16_t kRuntimeAutomationLanes =
      mclarrfile::kMaxAutomationLanes;
  mclarrfile::AutomationLane automation_lanes_[kRuntimeAutomationLanes];
  AutomationRuntimeLane automation_runtime_[kRuntimeAutomationLanes];
  uint16_t automation_lane_count_ = 0;
  uint32_t automation_point_count_ = 0;
  uint32_t automation_point_offset_ = 0;
  uint8_t automation_arrangement_idx_ = 0xFF;
  bool automation_cache_valid_ = false;
  bool automation_runtime_valid_ = false;
  static const uint8_t kAutomationPendingWrites = 32;
  AutomationPendingWrite automation_pending_writes_[kAutomationPendingWrites];
  uint8_t automation_pending_count_ = 0;
  uint32_t runtime_private_source_ids_[NUM_SLOTS] = {};
  uint8_t runtime_private_source_slots_[NUM_SLOTS] = {};
  uint32_t runtime_private_dirty_mask_ = 0;
  uint32_t queued_private_source_ids_[NUM_SLOTS] = {};
  bool host_playback_suspended_ = false;
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
