#include "MCLPlatformFeatures.h"

#if MCL_FEATURE_HOST_ARRANGER

#include "Arrangement/MCLArrangement.h"
#include "MCLArrangement_Internal.h"

using namespace mcl_arrangement_internal;

bool MCLArrangement::rewriteActive(const mclarrfile::Header &header,
                                   const mclarrfile::Clip *clips,
                                   uint32_t clipCount) {
  ScopedScratch<mclarrfile::Marker> markers(mclarrfile::kMaxMarkers);
  ScopedScratch<mclarrfile::LoopRegion> loopRegions(
      mclarrfile::kMaxLoopRegions);
  if (!markers || !loopRegions) {
    return false;
  }
  char labels[mclarrfile::kTrackLabelCount][mclarrfile::kTrackLabelBytes];
  uint16_t markerCount = 0;
  uint16_t loopRegionCount = 0;
  readActiveData(header, nullptr, nullptr, markers.get(), &markerCount,
                 labels, loopRegions.get(), &loopRegionCount);
  return rewriteActiveWithMetadata(header, clips, clipCount, markers.get(),
                                   markerCount, labels, loopRegions.get(),
                                   loopRegionCount);
}

bool MCLArrangement::rewriteActiveWithMarkers(
    const mclarrfile::Header &header, const mclarrfile::Clip *clips,
    uint32_t clipCount, const mclarrfile::Marker *markers,
    uint16_t markerCount) {
  char labels[mclarrfile::kTrackLabelCount][mclarrfile::kTrackLabelBytes];
  ScopedScratch<mclarrfile::LoopRegion> loopRegions(
      mclarrfile::kMaxLoopRegions);
  if (!loopRegions) {
    return false;
  }
  clearTrackLabels(labels);
  uint16_t loopRegionCount = 0;
  readActiveData(header, nullptr, nullptr, nullptr, nullptr, labels,
                 loopRegions.get(), &loopRegionCount);
  return rewriteActiveWithMetadata(header, clips, clipCount, markers,
                                   markerCount, labels, loopRegions.get(),
                                   loopRegionCount);
}

bool MCLArrangement::rewriteActiveWithMetadata(
    const mclarrfile::Header &header, const mclarrfile::Clip *clips,
    uint32_t clipCount, const mclarrfile::Marker *markers,
    uint16_t markerCount,
    const char trackLabels[mclarrfile::kTrackLabelCount]
                          [mclarrfile::kTrackLabelBytes],
    const mclarrfile::LoopRegion *loopRegions, uint16_t loopRegionCount,
    const AutomationWriteData *automationOverride) {
  if (!ensureArrangementDir()) {
    return false;
  }

  ScopedScratch<mclarrfile::AutomationLane> automationLanes;
  ScopedScratch<mclarrfile::AutomationPoint> automationPoints;
  AutomationChunkData automation;
  if (automationOverride != nullptr) {
    automation.lanes = const_cast<mclarrfile::AutomationLane *>(
        automationOverride->lanes);
    automation.points = const_cast<mclarrfile::AutomationPoint *>(
        automationOverride->points);
    automation.lane_count = automationOverride->lane_count;
    automation.point_count = automationOverride->point_count;
  } else {
    if (!automationLanes.allocate(mclarrfile::kMaxAutomationLanes) ||
        !automationPoints.allocate(mclarrfile::kMaxAutomationPoints)) {
      return false;
    }
    automation.lanes = automationLanes.get();
    automation.points = automationPoints.get();
    if (!readAutomationData(header, &automation)) {
      return false;
    }
  }

  File file;
  if (!openActive(&file, O_RDWR | O_CREAT)) {
    return false;
  }
  mclarrfile::Header outHeader = header;
  outHeader.version = mclarrfile::kVersion;
  bool haveLabels = trackLabelsHaveAny(trackLabels);
  bool haveLoops = loopRegions != nullptr && loopRegionCount > 0;
  bool haveAutomation =
      automation.lane_count > 0 || automation.point_count > 0;
  uint16_t chunkCount = 0;
  if (clipCount > 0) {
    ++chunkCount;
  }
  if (markerCount > 0) {
    ++chunkCount;
  }
  if (haveLabels) {
    ++chunkCount;
  }
  if (haveLoops) {
    ++chunkCount;
  }
  if (automation.lane_count > 0) {
    ++chunkCount;
  }
  if (automation.point_count > 0) {
    ++chunkCount;
  }
  bool haveExtra = chunkCount > 0;
  outHeader.headerBytes =
      haveExtra ? (uint16_t)(sizeof(mclarrfile::Header) +
                             sizeof(mclarrfile::HeaderExtraV7) +
                             chunkCount * sizeof(mclarrfile::ChunkDirEntry))
                : (uint16_t)sizeof(mclarrfile::Header);
  outHeader.clipBytes = sizeof(mclarrfile::Clip);
  outHeader.clipCount = clipCount;
  outHeader.flags &= (uint16_t)~(mclarrfile::HEADER_HAS_MARKERS |
                                 mclarrfile::HEADER_HAS_TRACK_LABELS |
                                 mclarrfile::HEADER_HAS_LOOP_REGIONS |
                                 mclarrfile::HEADER_HAS_AUTOMATION |
                                 mclarrfile::HEADER_HAS_CHUNKS);
  if (markerCount > 0) {
    outHeader.flags |= mclarrfile::HEADER_HAS_MARKERS;
  }
  if (haveLabels) {
    outHeader.flags |= mclarrfile::HEADER_HAS_TRACK_LABELS;
  }
  if (haveLoops) {
    outHeader.flags |= mclarrfile::HEADER_HAS_LOOP_REGIONS;
  }
  if (haveAutomation) {
    outHeader.flags |= mclarrfile::HEADER_HAS_AUTOMATION;
  }
  if (chunkCount > 0) {
    outHeader.flags |= mclarrfile::HEADER_HAS_CHUNKS;
  }

  mclarrfile::ChunkDirEntry chunks[6];
  memset(chunks, 0, sizeof(chunks));
  uint16_t chunkIndex = 0;
  uint32_t fileSize = outHeader.headerBytes;
  if (clipCount > 0) {
    chunks[chunkIndex].id = mclarrfile::CHUNK_CLIPS;
    chunks[chunkIndex].offset = fileSize;
    chunks[chunkIndex].count = clipCount;
    chunks[chunkIndex].itemBytes = sizeof(mclarrfile::Clip);
    fileSize += clipCount * sizeof(mclarrfile::Clip);
    ++chunkIndex;
  }
  if (markerCount > 0) {
    chunks[chunkIndex].id = mclarrfile::CHUNK_MARKERS;
    chunks[chunkIndex].offset = fileSize;
    chunks[chunkIndex].count = markerCount;
    chunks[chunkIndex].itemBytes = sizeof(mclarrfile::Marker);
    fileSize += markerCount * sizeof(mclarrfile::Marker);
    ++chunkIndex;
  }
  if (haveLabels) {
    chunks[chunkIndex].id = mclarrfile::CHUNK_TRACK_LABELS;
    chunks[chunkIndex].offset = fileSize;
    chunks[chunkIndex].count = mclarrfile::kTrackLabelCount;
    chunks[chunkIndex].itemBytes = mclarrfile::kTrackLabelBytes;
    fileSize += (uint32_t)mclarrfile::kTrackLabelCount *
                mclarrfile::kTrackLabelBytes;
    ++chunkIndex;
  }
  if (haveLoops) {
    chunks[chunkIndex].id = mclarrfile::CHUNK_LOOP_REGIONS;
    chunks[chunkIndex].offset = fileSize;
    chunks[chunkIndex].count = loopRegionCount;
    chunks[chunkIndex].itemBytes = sizeof(mclarrfile::LoopRegion);
    fileSize += (uint32_t)loopRegionCount *
                sizeof(mclarrfile::LoopRegion);
    ++chunkIndex;
  }
  if (automation.lane_count > 0) {
    chunks[chunkIndex].id = mclarrfile::CHUNK_AUTOMATION_LANES;
    chunks[chunkIndex].offset = fileSize;
    chunks[chunkIndex].count = automation.lane_count;
    chunks[chunkIndex].itemBytes = sizeof(mclarrfile::AutomationLane);
    fileSize += (uint32_t)automation.lane_count *
                sizeof(mclarrfile::AutomationLane);
    ++chunkIndex;
  }
  if (automation.point_count > 0) {
    chunks[chunkIndex].id = mclarrfile::CHUNK_AUTOMATION_POINTS;
    chunks[chunkIndex].offset = fileSize;
    chunks[chunkIndex].count = automation.point_count;
    chunks[chunkIndex].itemBytes = sizeof(mclarrfile::AutomationPoint);
    fileSize += automation.point_count * sizeof(mclarrfile::AutomationPoint);
    ++chunkIndex;
  }
  bool ok = file.seekSet(0) &&
            mcl_sd.write_data(&outHeader, sizeof(outHeader), &file);
  if (ok && haveExtra) {
    mclarrfile::HeaderExtraV7 extra;
    extra.markerBytes = markerCount > 0 ? sizeof(mclarrfile::Marker) : 0;
    extra.markerCount = markerCount;
    extra.trackLabelBytes =
        haveLabels ? mclarrfile::kTrackLabelBytes : (uint16_t)0;
    extra.trackLabelCount =
        haveLabels ? mclarrfile::kTrackLabelCount : (uint16_t)0;
    extra.loopRegionBytes =
        haveLoops ? sizeof(mclarrfile::LoopRegion) : (uint16_t)0;
    extra.loopRegionCount = haveLoops ? loopRegionCount : (uint16_t)0;
    extra.chunkDirBytes = sizeof(mclarrfile::ChunkDirEntry);
    extra.chunkCount = chunkCount;
    ok = mcl_sd.write_data(&extra, sizeof(extra), &file);
  }
  for (uint16_t i = 0; ok && i < chunkCount; ++i) {
    ok = mcl_sd.write_data((void *)&chunks[i], sizeof(chunks[i]), &file);
  }
  for (uint32_t i = 0; ok && i < clipCount; ++i) {
    ok = mcl_sd.write_data((void *)&clips[i], sizeof(clips[i]), &file);
  }
  for (uint16_t i = 0; ok && i < markerCount; ++i) {
    ok = mcl_sd.write_data((void *)&markers[i], sizeof(markers[i]), &file);
  }
  for (uint8_t i = 0; ok && haveLabels && i < mclarrfile::kTrackLabelCount;
       ++i) {
    ok = mcl_sd.write_data((void *)trackLabels[i],
                           mclarrfile::kTrackLabelBytes, &file);
  }
  for (uint16_t i = 0; ok && haveLoops && i < loopRegionCount; ++i) {
    ok = mcl_sd.write_data((void *)&loopRegions[i],
                           sizeof(loopRegions[i]), &file);
  }
  for (uint16_t i = 0; ok && i < automation.lane_count; ++i) {
    ok = mcl_sd.write_data((void *)&automation.lanes[i],
                           sizeof(mclarrfile::AutomationLane), &file);
  }
  for (uint32_t i = 0; ok && i < automation.point_count; ++i) {
    ok = mcl_sd.write_data((void *)&automation.points[i],
                           sizeof(mclarrfile::AutomationPoint), &file);
  }
  ok = ok && file.truncate(fileSize);
  ok = ok && file.sync();
  file.close();
  return ok;
}

#endif  // MCL_FEATURE_HOST_ARRANGER
