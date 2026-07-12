#include "MCLPlatformFeatures.h"

#if MCL_FEATURE_HOST_ARRANGER

#include "Arrangement/MCLArrangement.h"
#include "MCLArrangement_Internal.h"

using namespace mcl_arrangement_internal;

bool MCLArrangement::setTrackLabel(uint8_t track, const char *label) {
  if (track >= NUM_SLOTS || track >= mclarrfile::kTrackLabelCount) {
    return false;
  }
  if (!ensureActive()) {
    return false;
  }

  ScopedScratch<mclarrfile::Clip> clips(kMaxImportClips);
  if (!clips) {
    return false;
  }
  ScopedScratch<mclarrfile::Marker> markers(mclarrfile::kMaxMarkers);
  ScopedScratch<mclarrfile::LoopRegion> loopRegions(
      mclarrfile::kMaxLoopRegions);
  if (!markers || !loopRegions) {
    return false;
  }
  char labels[mclarrfile::kTrackLabelCount][mclarrfile::kTrackLabelBytes];
  uint32_t clipCount = 0;
  uint16_t markerCount = 0;
  uint16_t loopRegionCount = 0;

  mclarrfile::Header header;
  if (!readMeta(&header)) {
    return false;
  }
  if (!readActiveData(header, clips.get(), &clipCount, markers.get(),
                      &markerCount, labels, loopRegions.get(),
                      &loopRegionCount)) {
    return false;
  }

  sanitizeLabel(label, labels[track], mclarrfile::kTrackLabelBytes);
  bool ok = rewriteActiveWithMetadata(header, clips.get(), clipCount,
                                      markers.get(), markerCount, labels,
                                      loopRegions.get(), loopRegionCount);
  if (ok) {
    resetPlayback();
  }
  return ok;
}

bool MCLArrangement::setMarkerLabel(uint32_t startQ12, uint8_t track,
                                    const char *label) {
  if (track != spsarr::kArrMarkerGlobalTrack &&
      track >= NUM_SLOTS) {
    return false;
  }
  if (!ensureActive()) {
    return false;
  }

  ScopedScratch<mclarrfile::Clip> clips(kMaxImportClips);
  if (!clips) {
    return false;
  }
  ScopedScratch<mclarrfile::Marker> markers(mclarrfile::kMaxMarkers);
  ScopedScratch<mclarrfile::LoopRegion> loopRegions(
      mclarrfile::kMaxLoopRegions);
  if (!markers || !loopRegions) {
    return false;
  }
  char labels[mclarrfile::kTrackLabelCount][mclarrfile::kTrackLabelBytes];
  uint32_t clipCount = 0;
  uint16_t markerCount = 0;
  uint16_t loopRegionCount = 0;

  mclarrfile::Header header;
  if (!readMeta(&header)) {
    return false;
  }
  if (!readActiveData(header, clips.get(), &clipCount, markers.get(),
                      &markerCount, labels, loopRegions.get(),
                      &loopRegionCount)) {
    return false;
  }

  char cleanLabel[mclarrfile::kMarkerLabelBytes];
  sanitizeLabel(label, cleanLabel, sizeof(cleanLabel));
  int existing = -1;
  for (uint16_t i = 0; i < markerCount; ++i) {
    if (markers[i].track == track && markers[i].startQ12 == startQ12) {
      existing = i;
      break;
    }
  }

  if (cleanLabel[0] == '\0') {
    if (existing >= 0) {
      for (uint16_t i = (uint16_t)existing; i + 1 < markerCount; ++i) {
        markers[i] = markers[i + 1];
      }
      --markerCount;
    }
  } else {
    mclarrfile::Marker marker;
    memset(&marker, 0, sizeof(marker));
    marker.startQ12 = startQ12;
    marker.track = track;
    marker.flags = mclarrfile::MARKER_LABEL;
    memcpy(marker.label, cleanLabel, sizeof(marker.label));
    if (existing >= 0) {
      markers[existing] = marker;
    } else {
      if (markerCount >= mclarrfile::kMaxMarkers) {
        return false;
      }
      markers[markerCount++] = marker;
    }
  }

  sortMarkers(markers.get(), markerCount);
  bool ok = rewriteActiveWithMetadata(header, clips.get(), clipCount,
                                      markers.get(), markerCount, labels,
                                      loopRegions.get(), loopRegionCount);
  if (ok) {
    resetPlayback();
  }
  return ok;
}

bool MCLArrangement::setLoopRegionRecord(
    const mclarrfile::LoopRegion &region) {
  if (!ensureActive()) {
    return false;
  }

  ScopedScratch<mclarrfile::Clip> clips(kMaxImportClips);
  if (!clips) {
    return false;
  }
  ScopedScratch<mclarrfile::Marker> markers(mclarrfile::kMaxMarkers);
  ScopedScratch<mclarrfile::LoopRegion> loopRegions(
      mclarrfile::kMaxLoopRegions);
  if (!markers || !loopRegions) {
    return false;
  }
  char labels[mclarrfile::kTrackLabelCount][mclarrfile::kTrackLabelBytes];
  uint32_t clipCount = 0;
  uint16_t markerCount = 0;
  uint16_t loopRegionCount = 0;

  mclarrfile::Header header;
  if (!readMeta(&header)) {
    return false;
  }
  if (!readActiveData(header, clips.get(), &clipCount, markers.get(),
                      &markerCount, labels, loopRegions.get(),
                      &loopRegionCount)) {
    return false;
  }

  int existing = -1;
  if (region.id != 0) {
    for (uint16_t i = 0; i < loopRegionCount; ++i) {
      if (loopRegions[i].id == region.id) {
        existing = i;
        break;
      }
    }
  }
  if (existing < 0) {
    for (uint16_t i = 0; i < loopRegionCount; ++i) {
      if (loopRegions[i].startQ12 == region.startQ12) {
        existing = i;
        break;
      }
    }
  }

  bool enabled = (region.flags & mclarrfile::LOOP_REGION_ENABLED) != 0 &&
                 region.endQ12 > region.startQ12 &&
                 region.endQ12 - region.startQ12 >= spsarr::kMinArrLoopQ12;
  if (!enabled) {
    if (existing >= 0) {
      for (uint16_t i = (uint16_t)existing; i + 1 < loopRegionCount; ++i) {
        loopRegions[i] = loopRegions[i + 1];
      }
      --loopRegionCount;
    }
  } else {
    mclarrfile::LoopRegion clean;
    memset(&clean, 0, sizeof(clean));
    clean.startQ12 = region.startQ12;
    clean.endQ12 = region.endQ12;
    clean.repeatCount = region.repeatCount;
    clean.id = region.id;
    if (clean.id == 0) {
      uint16_t maxId = 0;
      for (uint16_t i = 0; i < loopRegionCount; ++i) {
        if (loopRegions[i].id > maxId) {
          maxId = loopRegions[i].id;
        }
      }
      clean.id = (uint16_t)(maxId + 1);
      if (clean.id == 0) {
        clean.id = 1;
      }
    }
    clean.flags = mclarrfile::LOOP_REGION_ENABLED;
    sanitizeLabel(region.label, clean.label, sizeof(clean.label));
    if (existing >= 0) {
      loopRegions[existing] = clean;
    } else {
      if (loopRegionCount >= mclarrfile::kMaxLoopRegions) {
        return false;
      }
      loopRegions[loopRegionCount++] = clean;
    }
  }

  sortLoopRegions(loopRegions.get(), loopRegionCount);
  bool ok = rewriteActiveWithMetadata(header, clips.get(), clipCount,
                                      markers.get(), markerCount, labels,
                                      loopRegions.get(), loopRegionCount);
  if (ok) {
    resetPlayback();
  }
  return ok;
}

bool MCLArrangement::setClipFade(uint32_t startQ12, uint32_t durationQ12,
                                 uint8_t track, uint8_t row, bool fadeOut,
                                 bool overrideFade,
                                 const TrackLoadFadeData &fade) {
  if (track >= NUM_SLOTS || row >= GRID_LENGTH || !ensureActive()) {
    return false;
  }

  ScopedScratch<mclarrfile::Clip> clips(kMaxImportClips);
  if (!clips) {
    return false;
  }
  ScopedScratch<mclarrfile::Marker> markers(mclarrfile::kMaxMarkers);
  ScopedScratch<mclarrfile::LoopRegion> loopRegions(
      mclarrfile::kMaxLoopRegions);
  if (!markers || !loopRegions) {
    return false;
  }
  char labels[mclarrfile::kTrackLabelCount][mclarrfile::kTrackLabelBytes];
  uint32_t clipCount = 0;
  uint16_t markerCount = 0;
  uint16_t loopRegionCount = 0;

  mclarrfile::Header header;
  if (!readMeta(&header)) {
    return false;
  }
  if (!readActiveData(header, clips.get(), &clipCount, markers.get(),
                      &markerCount, labels, loopRegions.get(),
                      &loopRegionCount)) {
    return false;
  }

  bool found = false;
  for (uint32_t i = 0; i < clipCount; ++i) {
    mclarrfile::Clip &clip = clips[i];
    if (clip.startQ12 != startQ12 || clip.durationQ12 != durationQ12 ||
        clip.track != track || clip.row != row) {
      continue;
    }
    ::setClipFade(clip, fade, fadeOut, overrideFade);
    found = true;
    break;
  }
  if (!found) {
    return false;
  }

  bool ok = rewriteActiveWithMetadata(header, clips.get(), clipCount,
                                      markers.get(), markerCount, labels,
                                      loopRegions.get(), loopRegionCount);
  if (ok) {
    reconcilePlaybackAfterEdit(false);
  }
  return ok;
}

bool MCLArrangement::replaceClips(const mclarrfile::Clip *clips,
                                  uint32_t clipCount) {
  if ((clipCount > 0 && clips == nullptr) || clipCount > kMaxImportClips ||
      !ensureActive()) {
    return false;
  }

  ScopedScratch<mclarrfile::Clip> cleanClips(clipCount);
  if (!cleanClips) {
    return false;
  }
  ScopedScratch<mclarrfile::Marker> markers(mclarrfile::kMaxMarkers);
  ScopedScratch<mclarrfile::LoopRegion> loopRegions(
      mclarrfile::kMaxLoopRegions);
  if (!markers || !loopRegions) {
    return false;
  }
  char labels[mclarrfile::kTrackLabelCount][mclarrfile::kTrackLabelBytes];
  uint16_t markerCount = 0;
  uint16_t loopRegionCount = 0;

  mclarrfile::Header header;
  if (!readMeta(&header)) {
    return false;
  }
  if (!readActiveData(header, nullptr, nullptr, markers.get(), &markerCount,
                      labels, loopRegions.get(), &loopRegionCount)) {
    return false;
  }

  for (uint32_t i = 0; i < clipCount; ++i) {
    mclarrfile::Clip clean = clips[i];
    if (clean.durationQ12 == 0 || clean.track >= NUM_SLOTS ||
        clean.row >= GRID_LENGTH) {
      return false;
    }
    if (clean.repeatQ12 == 0) {
      clean.repeatQ12 = clean.durationQ12;
    }
    clean.flags &=
        (uint8_t)(mclarrfile::CLIP_LOOP | mclarrfile::CLIP_MUTED |
                  mclarrfile::CLIP_LOAD_SOUND |
                  mclarrfile::CLIP_FADE_OVERRIDE);
    clean.reserved = 0;
    clean.fadeReserved = 0;
    clean.endFadeReserved = 0;
    clean.sourceReserved = 0;

    if (clean.sourceKind == mclarrfile::CLIP_SOURCE_PRIVATE) {
      if (clean.sourceId == 0) {
        return false;
      }
      uint8_t sourceSlot = 0;
      if (!mclarrfile::decodePrivateSourceSlot(clean.sourceFlags,
                                               sourceSlot)) {
        sourceSlot = clean.track;
        clean.sourceFlags = mclarrfile::encodePrivateSourceSlot(sourceSlot);
      }
      if (sourceSlot >= NUM_SLOTS) {
        return false;
      }
      clean.sourceTrack = sourceSlot;
    } else {
      uint8_t sourceSlot =
          clean.sourceTrack < mclarrfile::kGridSourceSlotCount
              ? clean.sourceTrack
              : clean.track;
      if (sourceSlot >= NUM_SLOTS) {
        return false;
      }
      mclarrfile::initGridSource(clean, sourceSlot);
    }

    cleanClips[i] = clean;
  }

  sortClips(cleanClips.get(), clipCount);
  bool ok = rewriteActiveWithMetadata(header, cleanClips.get(), clipCount,
                                      markers.get(), markerCount, labels,
                                      loopRegions.get(), loopRegionCount);
  if (ok) {
    reconcilePlaybackAfterEdit(true);
  }
  return ok;
}

bool MCLArrangement::setAutomationLanePoints(
    const mclarrfile::AutomationLane &lane,
    const mclarrfile::AutomationPoint *points, uint16_t pointCount) {
  if (lane.track >= NUM_SLOTS || pointCount > mclarrfile::kMaxAutomationPoints ||
      (pointCount > 0 && points == nullptr) || !ensureActive()) {
    return false;
  }

  for (uint16_t i = 1; i < pointCount; ++i) {
    if (points[i].q12 < points[i - 1].q12) {
      return false;
    }
  }

  ScopedScratch<mclarrfile::Clip> clips(kMaxImportClips);
  if (!clips) {
    return false;
  }
  ScopedScratch<mclarrfile::Marker> markers(mclarrfile::kMaxMarkers);
  ScopedScratch<mclarrfile::LoopRegion> loopRegions(
      mclarrfile::kMaxLoopRegions);
  if (!markers || !loopRegions) {
    return false;
  }
  char labels[mclarrfile::kTrackLabelCount][mclarrfile::kTrackLabelBytes];
  uint32_t clipCount = 0;
  uint16_t markerCount = 0;
  uint16_t loopRegionCount = 0;

  mclarrfile::Header header;
  if (!readMeta(&header)) {
    return false;
  }
  if (!readActiveData(header, clips.get(), &clipCount, markers.get(),
                      &markerCount, labels, loopRegions.get(),
                      &loopRegionCount)) {
    return false;
  }

  ScopedScratch<mclarrfile::AutomationLane> oldLanes(
      mclarrfile::kMaxAutomationLanes);
  ScopedScratch<mclarrfile::AutomationPoint> oldPoints(
      mclarrfile::kMaxAutomationPoints);
  if (!oldLanes || !oldPoints) {
    return false;
  }
  AutomationChunkData oldAutomation;
  oldAutomation.lanes = oldLanes.get();
  oldAutomation.points = oldPoints.get();
  if (!readAutomationData(header, &oldAutomation)) {
    return false;
  }

  ScopedScratch<mclarrfile::AutomationLane> newLanes(
      mclarrfile::kMaxAutomationLanes);
  ScopedScratch<mclarrfile::AutomationPoint> newPoints(
      mclarrfile::kMaxAutomationPoints);
  if (!newLanes || !newPoints) {
    return false;
  }
  uint16_t newLaneCount = 0;
  uint32_t newPointCount = 0;
  bool replace =
      pointCount > 0 && (lane.flags & mclarrfile::AUTOMATION_LANE_ENABLED) != 0;

  auto sameLane = [](const mclarrfile::AutomationLane &a,
                     const mclarrfile::AutomationLane &b) {
    return a.track == b.track && a.targetType == b.targetType &&
           a.device == b.device && a.targetIndex == b.targetIndex &&
           a.valueType == b.valueType;
  };

  for (uint16_t i = 0; i < oldAutomation.lane_count; ++i) {
    const mclarrfile::AutomationLane &src = oldAutomation.lanes[i];
    if (sameLane(src, lane)) {
      continue;
    }
    if (src.pointOffset > oldAutomation.point_count ||
        src.pointCount > oldAutomation.point_count - src.pointOffset) {
      continue;
    }
    if (newLaneCount >= mclarrfile::kMaxAutomationLanes ||
        src.pointCount > mclarrfile::kMaxAutomationPoints - newPointCount) {
      return false;
    }
    mclarrfile::AutomationLane dst = src;
    dst.pointOffset = newPointCount;
    newLanes[newLaneCount++] = dst;
    for (uint16_t p = 0; p < src.pointCount; ++p) {
      newPoints[newPointCount++] = oldAutomation.points[src.pointOffset + p];
    }
  }

  if (replace) {
    if (newLaneCount >= mclarrfile::kMaxAutomationLanes ||
        pointCount > mclarrfile::kMaxAutomationPoints - newPointCount) {
      return false;
    }
    mclarrfile::AutomationLane dst = lane;
    dst.flags |= mclarrfile::AUTOMATION_LANE_ENABLED;
    dst.pointOffset = newPointCount;
    dst.pointCount = pointCount;
    dst.reserved = 0;
    newLanes[newLaneCount++] = dst;
    for (uint16_t p = 0; p < pointCount; ++p) {
      mclarrfile::AutomationPoint clean = points[p];
      if (clean.interp != mclarrfile::AUTOMATION_INTERP_CURVE) {
        clean.interp = mclarrfile::AUTOMATION_INTERP_HOLD;
        clean.curve = 0;
      }
      newPoints[newPointCount++] = clean;
    }
  }

  AutomationWriteData writeData;
  writeData.lanes = newLanes.get();
  writeData.points = newPoints.get();
  writeData.lane_count = newLaneCount;
  writeData.point_count = newPointCount;
  bool ok = rewriteActiveWithMetadata(header, clips.get(), clipCount,
                                      markers.get(), markerCount, labels,
                                      loopRegions.get(), loopRegionCount,
                                      &writeData);
  if (ok) {
    resetPlayback();
    resetAutomationRuntime();
  }
  return ok;
}

#if MCL_FEATURE_HOST_ARRANGER_RECORD_HOOKS
static uint32_t recordTimeQ12(uint32_t q12) {
  return q12 == 0xFFFFFFFFu ? currentClockQ12() : q12;
}

bool MCLArrangement::setAutomationRecordArmed(bool armed) {
  bool changed = false;
  if (!armed && automation_record_armed_) {
    changed = finalizeRecordedClips();
    for (uint8_t i = 0; i < NUM_SLOTS; ++i) {
      clip_record_[i].active = false;
    }
  }
  automation_record_armed_ = armed;
  return changed;
}

bool MCLArrangement::automationRecordArmed() const {
  return automation_record_armed_;
}

bool MCLArrangement::recordGridSlotLoad(uint8_t dstTrack, GridSlot sourceSlot,
                                        GridRow row, uint32_t startQ12,
                                        uint32_t privateSourceId) {
  if (!automation_record_armed_ || dstTrack >= NUM_SLOTS ||
      sourceSlot >= mclarrfile::kGridSourceSlotCount || row >= GRID_LENGTH) {
    return false;
  }

  uint32_t start = recordTimeQ12(startQ12);
  bool changed = closeRecordedClip(dstTrack, start);

  SourceCell cell = readSourceCell(sourceSlot, row);
  if (!cell.active) {
    return changed;
  }

  ClipRecordState &state = clip_record_[dstTrack];
  state.active = true;
  state.startQ12 = start;
  state.sourceSlot = sourceSlot;
  state.row = row;
  state.sourceKind = privateSourceId != 0 ? mclarrfile::CLIP_SOURCE_PRIVATE
                                          : mclarrfile::CLIP_SOURCE_GRID;
  state.sourceId = privateSourceId;
  state.repeatQ12 = cell.durationQ12 != 0 ? cell.durationQ12 : 16u * 12u;
  state.flags = cell.loadSound ? mclarrfile::CLIP_LOAD_SOUND : 0;
  state.hasFade = cell.hasFade;
  state.fade = cell.fade;
  return true;
}

bool MCLArrangement::recordGridSlotClear(uint8_t dstTrack, uint32_t endQ12) {
  if (!automation_record_armed_ || dstTrack >= NUM_SLOTS) {
    return false;
  }
  uint32_t end = recordTimeQ12(endQ12);
  return closeRecordedClip(dstTrack, end);
}

bool MCLArrangement::finalizeRecordedClips(uint32_t endQ12) {
  uint32_t end = recordTimeQ12(endQ12);
  bool changed = false;
  for (uint8_t i = 0; i < NUM_SLOTS; ++i) {
    if (closeRecordedClip(i, end)) {
      changed = true;
    }
  }
  return changed;
}

bool MCLArrangement::closeRecordedClip(uint8_t dstTrack, uint32_t endQ12) {
  if (dstTrack >= NUM_SLOTS) {
    return false;
  }
  ClipRecordState &state = clip_record_[dstTrack];
  if (!state.active) {
    return false;
  }
  if (endQ12 <= state.startQ12) {
    state.active = false;
    return false;
  }

  mclarrfile::Clip clip;
  memset(&clip, 0, sizeof(clip));
  clearClipFade(clip);
  clip.startQ12 = state.startQ12;
  clip.durationQ12 = endQ12 - state.startQ12;
  clip.repeatQ12 = state.repeatQ12 != 0 ? state.repeatQ12 : clip.durationQ12;
  clip.track = dstTrack;
  clip.row = state.row;
  clip.flags = state.flags;
  clip.reserved = 0;
  if (state.sourceKind == mclarrfile::CLIP_SOURCE_PRIVATE &&
      state.sourceId != 0) {
    clip.sourceKind = mclarrfile::CLIP_SOURCE_PRIVATE;
    clip.sourceTrack = state.sourceSlot;
    clip.sourceFlags = mclarrfile::encodePrivateSourceSlot(state.sourceSlot);
    clip.sourceReserved = 0;
    clip.sourceId = state.sourceId;
  } else {
    mclarrfile::initGridSource(clip, state.sourceSlot);
  }
  if (state.hasFade) {
    mcl_arrangement_internal::setClipFade(clip, state.fade,
                                          state.fade.fade_out(), true);
  }

  bool ok = writeRecordedClip(clip);
  if (ok) {
    state.active = false;
  }
  return ok;
}

bool MCLArrangement::writeRecordedClip(const mclarrfile::Clip &recorded) {
  if (recorded.track >= NUM_SLOTS || recorded.durationQ12 == 0 ||
      !ensureActive()) {
    return false;
  }

  ScopedScratch<mclarrfile::Clip> clips(kMaxImportClips);
  ScopedScratch<mclarrfile::Clip> nextClips(kMaxImportClips);
  ScopedScratch<mclarrfile::Marker> markers(mclarrfile::kMaxMarkers);
  ScopedScratch<mclarrfile::LoopRegion> loopRegions(
      mclarrfile::kMaxLoopRegions);
  if (!clips || !nextClips || !markers || !loopRegions) {
    return false;
  }
  char labels[mclarrfile::kTrackLabelCount][mclarrfile::kTrackLabelBytes];
  uint32_t clipCount = 0;
  uint16_t markerCount = 0;
  uint16_t loopRegionCount = 0;

  mclarrfile::Header header;
  if (!readMeta(&header)) {
    return false;
  }
  if (!readActiveData(header, clips.get(), &clipCount, markers.get(),
                      &markerCount, labels, loopRegions.get(),
                      &loopRegionCount)) {
    return false;
  }

  uint32_t nextCount = 0;
  uint64_t recordStart = recorded.startQ12;
  uint64_t recordEnd = recordStart + recorded.durationQ12;
  if (recordEnd < recordStart) {
    recordEnd = 0xFFFFFFFFull;
  }

  auto appendClip = [&](const mclarrfile::Clip &clip) -> bool {
    if (nextCount >= kMaxImportClips) {
      return false;
    }
    nextClips[nextCount++] = clip;
    return true;
  };

  for (uint32_t i = 0; i < clipCount; ++i) {
    const mclarrfile::Clip &src = clips[i];
    if (src.track != recorded.track ||
        !clipOverlaps(src, recorded.startQ12,
                      recorded.startQ12 + recorded.durationQ12)) {
      if (!appendClip(src)) {
        return false;
      }
      continue;
    }

    uint64_t srcStart = src.startQ12;
    uint64_t srcEnd = srcStart + src.durationQ12;
    if (srcEnd < srcStart) {
      srcEnd = 0xFFFFFFFFull;
    }

    if (srcStart < recordStart) {
      mclarrfile::Clip left = src;
      left.durationQ12 = (uint32_t)(recordStart - srcStart);
      if (left.durationQ12 > 0 && !appendClip(left)) {
        return false;
      }
    }
    if (srcEnd > recordEnd && recordEnd <= 0xFFFFFFFFull) {
      mclarrfile::Clip right = src;
      right.startQ12 = (uint32_t)recordEnd;
      right.durationQ12 = (uint32_t)(srcEnd - recordEnd);
      if (right.durationQ12 > 0 && !appendClip(right)) {
        return false;
      }
    }
  }

  if (!appendClip(recorded)) {
    return false;
  }
  sortClips(nextClips.get(), nextCount);
  return rewriteActiveWithMetadata(header, nextClips.get(), nextCount,
                                   markers.get(), markerCount, labels,
                                   loopRegions.get(), loopRegionCount);
}

static uint16_t clamp_record_automation_value(uint16_t value,
                                              uint8_t valueType) {
  if (valueType == mclarrfile::AUTOMATION_VALUE_BOOL) {
    return value != 0 ? 1 : 0;
  }
  if (valueType == mclarrfile::AUTOMATION_VALUE_U14) {
    return value > 16383 ? 16383 : value;
  }
  return value > 127 ? 127 : value;
}

bool MCLArrangement::recordAutomationPoint(uint8_t track, uint8_t targetType,
                                           uint8_t device,
                                           uint8_t targetIndex,
                                           uint8_t valueType, uint16_t value,
                                           uint8_t interp, int8_t curve) {
  if (!automation_record_armed_ ||
      track >= NUM_SLOTS ||
      targetType > mclarrfile::AUTOMATION_TARGET_TEMPO ||
      device > 1 ||
      valueType > mclarrfile::AUTOMATION_VALUE_U14 ||
      !ensureActive()) {
    return false;
  }

  if (interp != mclarrfile::AUTOMATION_INTERP_CURVE) {
    interp = mclarrfile::AUTOMATION_INTERP_HOLD;
    curve = 0;
  }

  mclarrfile::AutomationLane lane;
  memset(&lane, 0, sizeof(lane));
  lane.track = track;
  lane.targetType = targetType;
  lane.device = device;
  lane.targetIndex = targetIndex;
  lane.valueType = valueType;
  lane.flags = mclarrfile::AUTOMATION_LANE_ENABLED;

  mclarrfile::AutomationPoint point;
  memset(&point, 0, sizeof(point));
  point.q12 = currentClockQ12();
  point.value = clamp_record_automation_value(value, valueType);
  point.interp = interp;
  point.curve = interp == mclarrfile::AUTOMATION_INTERP_CURVE ? curve : 0;

  ScopedScratch<mclarrfile::Clip> clips(kMaxImportClips);
  if (!clips) {
    return false;
  }
  ScopedScratch<mclarrfile::Marker> markers(mclarrfile::kMaxMarkers);
  ScopedScratch<mclarrfile::LoopRegion> loopRegions(
      mclarrfile::kMaxLoopRegions);
  if (!markers || !loopRegions) {
    return false;
  }
  char labels[mclarrfile::kTrackLabelCount][mclarrfile::kTrackLabelBytes];
  uint32_t clipCount = 0;
  uint16_t markerCount = 0;
  uint16_t loopRegionCount = 0;

  mclarrfile::Header header;
  if (!readMeta(&header)) {
    return false;
  }
  if (!readActiveData(header, clips.get(), &clipCount, markers.get(),
                      &markerCount, labels, loopRegions.get(),
                      &loopRegionCount)) {
    return false;
  }

  ScopedScratch<mclarrfile::AutomationLane> oldLanes(
      mclarrfile::kMaxAutomationLanes);
  ScopedScratch<mclarrfile::AutomationPoint> oldPoints(
      mclarrfile::kMaxAutomationPoints);
  if (!oldLanes || !oldPoints) {
    return false;
  }
  AutomationChunkData oldAutomation;
  oldAutomation.lanes = oldLanes.get();
  oldAutomation.points = oldPoints.get();
  if (!readAutomationData(header, &oldAutomation)) {
    return false;
  }

  auto sameLane = [](const mclarrfile::AutomationLane &a,
                     const mclarrfile::AutomationLane &b) {
    return a.track == b.track && a.targetType == b.targetType &&
           a.device == b.device && a.targetIndex == b.targetIndex &&
           a.valueType == b.valueType;
  };

  ScopedScratch<mclarrfile::AutomationPoint> mergedPoints(
      mclarrfile::kMaxAutomationPoints);
  if (!mergedPoints) {
    return false;
  }
  uint16_t mergedPointCount = 0;
  bool foundLane = false;
  for (uint16_t i = 0; i < oldAutomation.lane_count; ++i) {
    const mclarrfile::AutomationLane &src = oldAutomation.lanes[i];
    if (!sameLane(src, lane)) {
      continue;
    }
    foundLane = true;
    if (src.pointOffset > oldAutomation.point_count ||
        src.pointCount > oldAutomation.point_count - src.pointOffset) {
      return false;
    }

    bool inserted = false;
    uint32_t lastQ12 = 0;
    bool haveLast = false;
    for (uint16_t p = 0; p < src.pointCount; ++p) {
      const mclarrfile::AutomationPoint &oldPoint =
          oldAutomation.points[src.pointOffset + p];
      if (haveLast && oldPoint.q12 < lastQ12) {
        return false;
      }
      haveLast = true;
      lastQ12 = oldPoint.q12;

      if (!inserted && point.q12 < oldPoint.q12) {
        if (mergedPointCount >= mclarrfile::kMaxAutomationPoints) {
          return false;
        }
        mergedPoints[mergedPointCount++] = point;
        inserted = true;
      }
      if (oldPoint.q12 == point.q12) {
        if (!inserted) {
          if (mergedPointCount >= mclarrfile::kMaxAutomationPoints) {
            return false;
          }
          mergedPoints[mergedPointCount++] = point;
          inserted = true;
        }
        continue;
      }
      if (mergedPointCount >= mclarrfile::kMaxAutomationPoints) {
        return false;
      }
      mergedPoints[mergedPointCount++] = oldPoint;
    }
    if (!inserted) {
      if (mergedPointCount >= mclarrfile::kMaxAutomationPoints) {
        return false;
      }
      mergedPoints[mergedPointCount++] = point;
    }
    break;
  }

  if (!foundLane) {
    mergedPoints[0] = point;
    mergedPointCount = 1;
  }

  ScopedScratch<mclarrfile::AutomationLane> newLanes(
      mclarrfile::kMaxAutomationLanes);
  ScopedScratch<mclarrfile::AutomationPoint> newPoints(
      mclarrfile::kMaxAutomationPoints);
  if (!newLanes || !newPoints) {
    return false;
  }
  uint16_t newLaneCount = 0;
  uint32_t newPointCount = 0;
  for (uint16_t i = 0; i < oldAutomation.lane_count; ++i) {
    const mclarrfile::AutomationLane &src = oldAutomation.lanes[i];
    if (sameLane(src, lane)) {
      continue;
    }
    if (src.pointOffset > oldAutomation.point_count ||
        src.pointCount > oldAutomation.point_count - src.pointOffset) {
      continue;
    }
    if (newLaneCount >= mclarrfile::kMaxAutomationLanes ||
        src.pointCount > mclarrfile::kMaxAutomationPoints - newPointCount) {
      return false;
    }
    mclarrfile::AutomationLane dst = src;
    dst.pointOffset = newPointCount;
    newLanes[newLaneCount++] = dst;
    for (uint16_t p = 0; p < src.pointCount; ++p) {
      newPoints[newPointCount++] =
          oldAutomation.points[src.pointOffset + p];
    }
  }

  if (newLaneCount >= mclarrfile::kMaxAutomationLanes ||
      mergedPointCount > mclarrfile::kMaxAutomationPoints - newPointCount) {
    return false;
  }
  mclarrfile::AutomationLane dst = lane;
  dst.pointOffset = newPointCount;
  dst.pointCount = mergedPointCount;
  dst.reserved = 0;
  newLanes[newLaneCount++] = dst;
  for (uint16_t p = 0; p < mergedPointCount; ++p) {
    newPoints[newPointCount++] = mergedPoints[p];
  }

  AutomationWriteData writeData;
  writeData.lanes = newLanes.get();
  writeData.points = newPoints.get();
  writeData.lane_count = newLaneCount;
  writeData.point_count = newPointCount;
  bool ok = rewriteActiveWithMetadata(header, clips.get(), clipCount,
                                      markers.get(), markerCount, labels,
                                      loopRegions.get(), loopRegionCount,
                                      &writeData);
  if (ok) {
    resetAutomationRuntime();
  }
  return ok;
}
#endif  // MCL_FEATURE_HOST_ARRANGER_RECORD_HOOKS

bool MCLArrangement::importGrid(uint16_t trackMask, uint8_t startRow) {
  if (trackMask == 0) {
    trackMask = 0xFFFF;
  }
  mclarrfile::Header header;
  if (!readMeta(&header)) {
    mclarrfile::initHeader(header, "main");
  }

  ScopedScratch<mclarrfile::Clip> clips(kMaxImportClips);
  if (!clips) {
    return false;
  }
  uint32_t count = 0;
  for (uint8_t track = 0; track < 16 && count < kMaxImportClips; ++track) {
    if (((trackMask >> track) & 1u) == 0) {
      continue;
    }
    GridRow baseRow = resolveTrackBaseRow(track, startRow);
    appendImportLane(track, baseRow, clips.get(), count, kMaxImportClips);
  }
  sortClips(clips.get(), count);
  bool ok = rewriteActive(header, clips.get(), count);
  if (ok) {
    resetPlayback();
    clearLoopRegion();
  }
  return ok;
}

#endif  // MCL_FEATURE_HOST_ARRANGER
