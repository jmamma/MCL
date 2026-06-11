#include "MCLPlatformFeatures.h"

#if MCL_FEATURE_HOST_ARRANGER

#include "Arrangement/MCLArrangement.h"
#include "MCLArrangement_Internal.h"

using namespace mcl_arrangement_internal;

bool MCLArrangement::rewriteActive(const mclarrfile::Header &header,
                                   const mclarrfile::Clip *clips,
                                   uint32_t clipCount) {
  static mclarrfile::Marker markers[mclarrfile::kMaxMarkers];
  static mclarrfile::LoopRegion loopRegions[mclarrfile::kMaxLoopRegions];
  char labels[mclarrfile::kTrackLabelCount][mclarrfile::kTrackLabelBytes];
  uint16_t markerCount = 0;
  uint16_t loopRegionCount = 0;
  readActiveData(header, nullptr, nullptr, markers, &markerCount, labels,
                 loopRegions, &loopRegionCount);
  return rewriteActiveWithMetadata(header, clips, clipCount, markers,
                                   markerCount, labels, loopRegions,
                                   loopRegionCount);
}

bool MCLArrangement::rewriteActiveWithMarkers(
    const mclarrfile::Header &header, const mclarrfile::Clip *clips,
    uint32_t clipCount, const mclarrfile::Marker *markers,
    uint16_t markerCount) {
  char labels[mclarrfile::kTrackLabelCount][mclarrfile::kTrackLabelBytes];
  static mclarrfile::LoopRegion loopRegions[mclarrfile::kMaxLoopRegions];
  clearTrackLabels(labels);
  uint16_t loopRegionCount = 0;
  readActiveData(header, nullptr, nullptr, nullptr, nullptr, labels,
                 loopRegions, &loopRegionCount);
  return rewriteActiveWithMetadata(header, clips, clipCount, markers,
                                   markerCount, labels, loopRegions,
                                   loopRegionCount);
}

bool MCLArrangement::rewriteActiveWithMetadata(
    const mclarrfile::Header &header, const mclarrfile::Clip *clips,
    uint32_t clipCount, const mclarrfile::Marker *markers,
    uint16_t markerCount,
    const char trackLabels[mclarrfile::kTrackLabelCount]
                          [mclarrfile::kTrackLabelBytes],
    const mclarrfile::LoopRegion *loopRegions, uint16_t loopRegionCount) {
  if (!ensureArrangementDir()) {
    return false;
  }
  File file;
  if (!openActive(&file, O_RDWR | O_CREAT)) {
    return false;
  }
  mclarrfile::Header outHeader = header;
  outHeader.version = mclarrfile::kVersion;
  bool haveLabels = trackLabelsHaveAny(trackLabels);
  bool haveLoops = loopRegions != nullptr && loopRegionCount > 0;
  bool haveExtra = markerCount > 0 || haveLabels || haveLoops;
  outHeader.headerBytes = haveExtra
                              ? (uint16_t)(sizeof(mclarrfile::Header) +
                                           sizeof(mclarrfile::HeaderExtraV6))
                              : (uint16_t)sizeof(mclarrfile::Header);
  outHeader.clipBytes = sizeof(mclarrfile::Clip);
  outHeader.clipCount = clipCount;
  if (markerCount > 0) {
    outHeader.flags |= mclarrfile::HEADER_HAS_MARKERS;
  } else {
    outHeader.flags &= (uint16_t)~mclarrfile::HEADER_HAS_MARKERS;
  }
  if (haveLabels) {
    outHeader.flags |= mclarrfile::HEADER_HAS_TRACK_LABELS;
  } else {
    outHeader.flags &= (uint16_t)~mclarrfile::HEADER_HAS_TRACK_LABELS;
  }
  if (haveLoops) {
    outHeader.flags |= mclarrfile::HEADER_HAS_LOOP_REGIONS;
  } else {
    outHeader.flags &= (uint16_t)~mclarrfile::HEADER_HAS_LOOP_REGIONS;
  }
  bool ok = file.seekSet(0) &&
            mcl_sd.write_data(&outHeader, sizeof(outHeader), &file);
  if (ok && haveExtra) {
    mclarrfile::HeaderExtraV6 extra;
    extra.markerBytes = markerCount > 0 ? sizeof(mclarrfile::Marker) : 0;
    extra.markerCount = markerCount;
    extra.trackLabelBytes =
        haveLabels ? mclarrfile::kTrackLabelBytes : (uint16_t)0;
    extra.trackLabelCount =
        haveLabels ? mclarrfile::kTrackLabelCount : (uint16_t)0;
    extra.loopRegionBytes =
        haveLoops ? sizeof(mclarrfile::LoopRegion) : (uint16_t)0;
    extra.loopRegionCount = haveLoops ? loopRegionCount : (uint16_t)0;
    ok = mcl_sd.write_data(&extra, sizeof(extra), &file);
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
  uint32_t fileSize = sizeof(mclarrfile::Header) +
                      (haveExtra ? sizeof(mclarrfile::HeaderExtraV6) : 0) +
                      clipCount * sizeof(mclarrfile::Clip) +
                      markerCount * sizeof(mclarrfile::Marker) +
                      (haveLabels ? (uint32_t)mclarrfile::kTrackLabelCount *
                                        mclarrfile::kTrackLabelBytes
                                  : 0) +
                      (haveLoops ? (uint32_t)loopRegionCount *
                                       sizeof(mclarrfile::LoopRegion)
                                 : 0);
  ok = ok && file.truncate(fileSize);
  ok = ok && file.sync();
  file.close();
  return ok;
}

#endif  // MCL_FEATURE_HOST_ARRANGER
