#include "MCLPlatformFeatures.h"

#if MCL_FEATURE_HOST_ARRANGER

#include "MCLArrangement.h"
#include "MCLArrangement_Internal.h"

using namespace mcl_arrangement_internal;

bool MCLArrangement::setTrackLabel(uint8_t track, const char *label) {
  if (track >= NUM_SLOTS || track >= mclarrfile::kTrackLabelCount) {
    return false;
  }
  if (!ensureActive()) {
    return false;
  }

  static mclarrfile::Clip clips[kMaxImportClips];
  static mclarrfile::Marker markers[mclarrfile::kMaxMarkers];
  static mclarrfile::LoopRegion loopRegions[mclarrfile::kMaxLoopRegions];
  char labels[mclarrfile::kTrackLabelCount][mclarrfile::kTrackLabelBytes];
  uint32_t clipCount = 0;
  uint16_t markerCount = 0;
  uint16_t loopRegionCount = 0;

  mclarrfile::Header header;
  if (!readMeta(&header)) {
    return false;
  }
  if (!readActiveData(header, clips, &clipCount, markers, &markerCount,
                      labels, loopRegions, &loopRegionCount)) {
    return false;
  }

  sanitizeLabel(label, labels[track], mclarrfile::kTrackLabelBytes);
  bool ok = rewriteActiveWithMetadata(header, clips, clipCount, markers,
                                      markerCount, labels, loopRegions,
                                      loopRegionCount);
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

  static mclarrfile::Clip clips[kMaxImportClips];
  static mclarrfile::Marker markers[mclarrfile::kMaxMarkers];
  static mclarrfile::LoopRegion loopRegions[mclarrfile::kMaxLoopRegions];
  char labels[mclarrfile::kTrackLabelCount][mclarrfile::kTrackLabelBytes];
  uint32_t clipCount = 0;
  uint16_t markerCount = 0;
  uint16_t loopRegionCount = 0;

  mclarrfile::Header header;
  if (!readMeta(&header)) {
    return false;
  }
  if (!readActiveData(header, clips, &clipCount, markers, &markerCount,
                      labels, loopRegions, &loopRegionCount)) {
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

  sortMarkers(markers, markerCount);
  bool ok = rewriteActiveWithMetadata(header, clips, clipCount, markers,
                                      markerCount, labels, loopRegions,
                                      loopRegionCount);
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

  static mclarrfile::Clip clips[kMaxImportClips];
  static mclarrfile::Marker markers[mclarrfile::kMaxMarkers];
  static mclarrfile::LoopRegion loopRegions[mclarrfile::kMaxLoopRegions];
  char labels[mclarrfile::kTrackLabelCount][mclarrfile::kTrackLabelBytes];
  uint32_t clipCount = 0;
  uint16_t markerCount = 0;
  uint16_t loopRegionCount = 0;

  mclarrfile::Header header;
  if (!readMeta(&header)) {
    return false;
  }
  if (!readActiveData(header, clips, &clipCount, markers, &markerCount,
                      labels, loopRegions, &loopRegionCount)) {
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

  sortLoopRegions(loopRegions, loopRegionCount);
  bool ok = rewriteActiveWithMetadata(header, clips, clipCount, markers,
                                      markerCount, labels, loopRegions,
                                      loopRegionCount);
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

  static mclarrfile::Clip clips[kMaxImportClips];
  static mclarrfile::Marker markers[mclarrfile::kMaxMarkers];
  static mclarrfile::LoopRegion loopRegions[mclarrfile::kMaxLoopRegions];
  char labels[mclarrfile::kTrackLabelCount][mclarrfile::kTrackLabelBytes];
  uint32_t clipCount = 0;
  uint16_t markerCount = 0;
  uint16_t loopRegionCount = 0;

  mclarrfile::Header header;
  if (!readMeta(&header)) {
    return false;
  }
  if (!readActiveData(header, clips, &clipCount, markers, &markerCount,
                      labels, loopRegions, &loopRegionCount)) {
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

  bool ok = rewriteActiveWithMetadata(header, clips, clipCount, markers,
                                      markerCount, labels, loopRegions,
                                      loopRegionCount);
  if (ok) {
    resetPlayback();
  }
  return ok;
}

bool MCLArrangement::importGrid(uint16_t trackMask, uint8_t startRow) {
  if (trackMask == 0) {
    trackMask = 0xFFFF;
  }
  mclarrfile::Header header;
  if (!readMeta(&header)) {
    mclarrfile::initHeader(header, "main");
  }

  static mclarrfile::Clip clips[kMaxImportClips];
  uint32_t count = 0;
  for (uint8_t track = 0; track < 16 && count < kMaxImportClips; ++track) {
    if (((trackMask >> track) & 1u) == 0) {
      continue;
    }
    GridRow baseRow = resolveTrackBaseRow(track, startRow);
    appendImportLane(track, baseRow, clips, count, kMaxImportClips);
  }
  sortClips(clips, count);
  bool ok = rewriteActive(header, clips, count);
  if (ok) {
    resetPlayback();
    clearLoopRegion();
  }
  return ok;
}

#endif  // MCL_FEATURE_HOST_ARRANGER
