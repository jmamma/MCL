#include "MCLPlatformFeatures.h"

#if MCL_FEATURE_HOST_ARRANGER

#include "Arrangement/MCLArrangement.h"
#include "MCLArrangement_Internal.h"

using namespace mcl_arrangement_internal;

uint16_t MCLArrangement::readClips(uint32_t startQ12, uint32_t endQ12,
                                   uint16_t skip, uint8_t maxClips,
                                   mclarrfile::Clip *out,
                                   uint32_t *totalMatches, bool *more) {
  if (totalMatches != nullptr) {
    *totalMatches = 0;
  }
  if (more != nullptr) {
    *more = false;
  }
  if (out == nullptr || maxClips == 0) {
    return 0;
  }

  mclarrfile::Header header;
  if (!readMeta(&header)) {
    return 0;
  }
  File file;
  if (!openActive(&file, O_READ)) {
    return 0;
  }
  if (!file.seekSet(header.headerBytes)) {
    file.close();
    return 0;
  }

  uint16_t returned = 0;
  uint32_t matched = 0;
  for (uint32_t i = 0; i < header.clipCount; ++i) {
    mclarrfile::Clip clip;
    if (!readClipRecord(file, header, clip)) {
      break;
    }
    if (!clipOverlaps(clip, startQ12, endQ12)) {
      continue;
    }
    if (matched++ < skip) {
      continue;
    }
    if (returned < maxClips) {
      out[returned++] = clip;
    } else if (more != nullptr) {
      *more = true;
    }
  }
  file.close();
  if (totalMatches != nullptr) {
    *totalMatches = matched;
  }
  return returned;
}

uint16_t MCLArrangement::readMarkers(uint32_t startQ12, uint32_t endQ12,
                                     uint16_t skip, uint8_t maxMarkers,
                                     mclarrfile::Marker *out,
                                     uint32_t *totalMatches, bool *more) {
  if (totalMatches != nullptr) {
    *totalMatches = 0;
  }
  if (more != nullptr) {
    *more = false;
  }
  if (out == nullptr || maxMarkers == 0) {
    return 0;
  }

  mclarrfile::Header header;
  if (!readMeta(&header)) {
    return 0;
  }
  if ((header.flags & mclarrfile::HEADER_HAS_MARKERS) == 0 ||
      header.headerBytes < sizeof(mclarrfile::Header) +
                               sizeof(mclarrfile::HeaderExtraV2)) {
    return 0;
  }

  File file;
  if (!openActive(&file, O_READ)) {
    return 0;
  }

  mclarrfile::HeaderExtraV6 extra;
  bool ok = file.seekSet(sizeof(mclarrfile::Header)) &&
            readHeaderExtra(file, header, &extra) &&
            extra.markerBytes == sizeof(mclarrfile::Marker);
  if (!ok) {
    file.close();
    return 0;
  }

  uint32_t markerOffset = header.headerBytes +
                          header.clipCount * header.clipBytes;
  if (!file.seekSet(markerOffset)) {
    file.close();
    return 0;
  }

  uint16_t returned = 0;
  uint32_t matched = 0;
  for (uint16_t i = 0; i < extra.markerCount; ++i) {
    mclarrfile::Marker marker;
    if (!mcl_sd.read_data(&marker, sizeof(marker), &file)) {
      break;
    }
    if (!markerInRange(marker, startQ12, endQ12)) {
      continue;
    }
    if (matched++ < skip) {
      continue;
    }
    if (returned < maxMarkers) {
      out[returned++] = marker;
    } else if (more != nullptr) {
      *more = true;
    }
  }
  file.close();
  if (totalMatches != nullptr) {
    *totalMatches = matched;
  }
  return returned;
}

uint16_t MCLArrangement::readLoopRegions(uint32_t startQ12, uint32_t endQ12,
                                         uint16_t skip,
                                         uint8_t maxLoopRegions,
                                         mclarrfile::LoopRegion *out,
                                         uint32_t *totalMatches,
                                         bool *more) {
  if (totalMatches != nullptr) {
    *totalMatches = 0;
  }
  if (more != nullptr) {
    *more = false;
  }
  if (out == nullptr || maxLoopRegions == 0) {
    return 0;
  }

  mclarrfile::Header header;
  if (!readMeta(&header)) {
    return 0;
  }
  if ((header.flags & mclarrfile::HEADER_HAS_LOOP_REGIONS) == 0 ||
      header.headerBytes < sizeof(mclarrfile::Header) +
                               sizeof(mclarrfile::HeaderExtraV2)) {
    return 0;
  }

  File file;
  if (!openActive(&file, O_READ)) {
    return 0;
  }

  mclarrfile::HeaderExtraV6 extra;
  bool ok = readHeaderExtra(file, header, &extra) &&
            extra.loopRegionBytes == sizeof(mclarrfile::LoopRegion);
  if (!ok) {
    file.close();
    return 0;
  }

  uint32_t loopOffset =
      header.headerBytes + header.clipCount * header.clipBytes +
      (uint32_t)extra.markerCount * extra.markerBytes +
      ((header.flags & mclarrfile::HEADER_HAS_TRACK_LABELS) != 0
           ? (uint32_t)extra.trackLabelCount * extra.trackLabelBytes
           : 0u);
  if (!file.seekSet(loopOffset)) {
    file.close();
    return 0;
  }

  uint16_t returned = 0;
  uint32_t matched = 0;
  for (uint16_t i = 0; i < extra.loopRegionCount; ++i) {
    mclarrfile::LoopRegion region;
    if (!mcl_sd.read_data(&region, sizeof(region), &file)) {
      break;
    }
    if (!loopRegionInRange(region, startQ12, endQ12)) {
      continue;
    }
    if (matched++ < skip) {
      continue;
    }
    if (returned < maxLoopRegions) {
      out[returned++] = region;
    } else if (more != nullptr) {
      *more = true;
    }
  }
  file.close();
  if (totalMatches != nullptr) {
    *totalMatches = matched;
  }
  return returned;
}

bool MCLArrangement::readTrackLabels(
    char labels[mclarrfile::kTrackLabelCount][mclarrfile::kTrackLabelBytes]) {
  clearTrackLabels(labels);
  if (!ensureActive()) {
    return false;
  }
  mclarrfile::Header header;
  if (!readMeta(&header)) {
    return false;
  }
  return readActiveData(header, nullptr, nullptr, nullptr, nullptr, labels);
}

uint16_t MCLArrangement::readAutomationLanes(
    uint16_t skip, uint8_t maxLanes, mclarrfile::AutomationLane *out,
    uint32_t *totalMatches, bool *more) {
  if (totalMatches != nullptr) {
    *totalMatches = 0;
  }
  if (more != nullptr) {
    *more = false;
  }
  if (out == nullptr || maxLanes == 0) {
    return 0;
  }

  mclarrfile::Header header;
  if (!readMeta(&header) ||
      (header.flags & mclarrfile::HEADER_HAS_AUTOMATION) == 0) {
    return 0;
  }

  File file;
  if (!openActive(&file, O_READ)) {
    return 0;
  }

  mclarrfile::ChunkDirEntry chunk;
  bool ok = findChunk(file, header, mclarrfile::CHUNK_AUTOMATION_LANES,
                      &chunk) &&
            chunk.itemBytes == sizeof(mclarrfile::AutomationLane) &&
            chunk.count <= mclarrfile::kMaxAutomationLanes;
  if (!ok) {
    file.close();
    return 0;
  }

  if (totalMatches != nullptr) {
    *totalMatches = chunk.count;
  }
  if (skip >= chunk.count) {
    file.close();
    return 0;
  }

  uint32_t remaining = chunk.count - skip;
  uint16_t count = remaining < maxLanes ? (uint16_t)remaining : maxLanes;
  if (more != nullptr) {
    *more = remaining > count;
  }
  ok = file.seekSet(chunk.offset +
                    (uint32_t)skip * sizeof(mclarrfile::AutomationLane));
  for (uint16_t i = 0; ok && i < count; ++i) {
    ok = mcl_sd.read_data(&out[i], sizeof(mclarrfile::AutomationLane), &file);
  }
  file.close();
  return ok ? count : 0;
}

uint16_t MCLArrangement::readAutomationPoints(
    uint32_t pointOffset, uint16_t pointCount, uint16_t skip,
    uint8_t maxPoints, mclarrfile::AutomationPoint *out,
    uint32_t *totalMatches, bool *more) {
  if (totalMatches != nullptr) {
    *totalMatches = 0;
  }
  if (more != nullptr) {
    *more = false;
  }
  if (out == nullptr || maxPoints == 0 || pointCount == 0) {
    return 0;
  }

  mclarrfile::Header header;
  if (!readMeta(&header) ||
      (header.flags & mclarrfile::HEADER_HAS_AUTOMATION) == 0) {
    return 0;
  }

  File file;
  if (!openActive(&file, O_READ)) {
    return 0;
  }

  mclarrfile::ChunkDirEntry chunk;
  bool ok = findChunk(file, header, mclarrfile::CHUNK_AUTOMATION_POINTS,
                      &chunk) &&
            chunk.itemBytes == sizeof(mclarrfile::AutomationPoint) &&
            chunk.count <= mclarrfile::kMaxAutomationPoints &&
            pointOffset <= chunk.count &&
            pointCount <= chunk.count - pointOffset;
  if (!ok) {
    file.close();
    return 0;
  }

  if (totalMatches != nullptr) {
    *totalMatches = pointCount;
  }
  if (skip >= pointCount) {
    file.close();
    return 0;
  }

  uint32_t remaining = (uint32_t)pointCount - skip;
  uint16_t count = remaining < maxPoints ? (uint16_t)remaining : maxPoints;
  if (more != nullptr) {
    *more = remaining > count;
  }
  ok = file.seekSet(chunk.offset +
                    (pointOffset + skip) *
                        sizeof(mclarrfile::AutomationPoint));
  for (uint16_t i = 0; ok && i < count; ++i) {
    ok = mcl_sd.read_data(&out[i], sizeof(mclarrfile::AutomationPoint),
                          &file);
  }
  file.close();
  return ok ? count : 0;
}

#endif  // MCL_FEATURE_HOST_ARRANGER
