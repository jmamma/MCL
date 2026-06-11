#include "MCLPlatformFeatures.h"

#if MCL_FEATURE_HOST_ARRANGER

#include "MCLArrangement.h"
#include "MCLArrangement_Internal.h"

using namespace mcl_arrangement_internal;

bool MCLArrangement::makeClipLocal(uint32_t startQ12, uint32_t durationQ12,
                                   uint8_t track, uint8_t row,
                                   GridSlot expectedSourceSlot) {
  if (track >= NUM_SLOTS || row >= GRID_LENGTH || durationQ12 == 0 ||
      !ensureActive()) {
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

  int clipIndex = -1;
  for (uint32_t i = 0; i < clipCount; ++i) {
    mclarrfile::Clip &clip = clips[i];
    if (clip.startQ12 != startQ12 || clip.durationQ12 != durationQ12 ||
        clip.track != track || clip.row != row ||
        clip.sourceKind == mclarrfile::CLIP_SOURCE_PRIVATE) {
      continue;
    }
    GridSlot sourceSlot = (GridSlot)mclarrfile::clipSourceSlot(clip);
    if (expectedSourceSlot < NUM_SLOTS && sourceSlot != expectedSourceSlot) {
      continue;
    }
    clipIndex = (int)i;
    break;
  }
  if (clipIndex < 0) {
    return false;
  }

  uint32_t sourceId = nextPrivateSourceId(clips, clipCount);
  GridColumn localCol = 0;
  GridRow localRow = 0;
  if (!privateSourceCell(sourceId, &localCol, &localRow)) {
    return false;
  }

  mclarrfile::Clip &clip = clips[clipIndex];
  GridSlot sourceSlot = (GridSlot)mclarrfile::clipSourceSlot(clip);
  EmptyTrack scratch;
  DeviceTrack *source = scratch.load_from_grid_512(sourceSlot, clip.row);
  if (source == nullptr || !source->is_active()) {
    return false;
  }
  source->on_copy(sourceSlot & 0x0F, clip.track & 0x0F, false);

  Grid privateGrid;
  if (!openPrivateGrid(privateGrid, true)) {
    return false;
  }
  bool stored = source->store_in_grid(localCol, localRow, nullptr, 0, false,
                                      &privateGrid);
  stored = stored && privateGrid.sync();
  privateGrid.close_file();
  if (!stored) {
    return false;
  }

  clip.sourceKind = mclarrfile::CLIP_SOURCE_PRIVATE;
  clip.sourceTrack = clip.track;
  clip.sourceFlags = 0;
  clip.sourceReserved = 0;
  clip.sourceId = sourceId;

  bool ok = rewriteActiveWithMetadata(header, clips, clipCount, markers,
                                      markerCount, labels, loopRegions,
                                      loopRegionCount);
  if (ok) {
    resetPlayback();
  }
  return ok;
}

bool MCLArrangement::exportPrivateSourceToGrid(uint32_t sourceId,
                                               GridSlot sourceSlot,
                                               GridRow sourceRow,
                                               GridSlot targetSlot,
                                               GridRow targetRow) {
  if (sourceSlot >= NUM_SLOTS || targetSlot >= NUM_SLOTS ||
      sourceRow >= GRID_LENGTH || targetRow >= GRID_LENGTH) {
    return false;
  }

  GridColumn localCol = 0;
  GridRow localRow = 0;
  if (!privateSourceCell(sourceId, &localCol, &localRow)) {
    return false;
  }

  Grid privateGrid;
  if (!openPrivateGrid(privateGrid, false)) {
    return false;
  }

  EmptyTrack scratch;
  DeviceTrack *source = scratch.load_from_grid_512(localCol, localRow,
                                                   &privateGrid);
  privateGrid.close_file();
  if (source == nullptr || !source->is_active()) {
    return false;
  }

  int16_t linkRowOffset = (int16_t)source->link.row - (int16_t)sourceRow;
  int16_t newLinkRow = (int16_t)targetRow + linkRowOffset;
  if (newLinkRow < 0 || newLinkRow >= (int16_t)GRID_LENGTH) {
    newLinkRow = targetRow;
  }
  source->link.row = (uint8_t)newLinkRow;
  source->on_copy(sourceSlot & 0x0F, targetSlot & 0x0F, false);
  bool stored = source->store_in_grid(targetSlot, targetRow, nullptr, 0, false);
  if (!stored) {
    return false;
  }

  GridIndex targetGrid = targetSlot / GRID_WIDTH;
  GridRowHeader header;
  if (proj.read_grid_row_header(&header, targetRow, targetGrid)) {
    header.active = true;
    header.name[0] = '\0';
    proj.write_grid_row_header(&header, targetRow, targetGrid);
  }
  proj.sync_grid(targetGrid);
  return true;
}

void MCLArrangement::beginQueuedPrivateLoads(
    const uint32_t sourceIds[NUM_SLOTS]) {
  if (sourceIds == nullptr) {
    memset(queued_private_source_ids_, 0, sizeof(queued_private_source_ids_));
    return;
  }
  memcpy(queued_private_source_ids_, sourceIds,
         sizeof(queued_private_source_ids_));
}

void MCLArrangement::endQueuedPrivateLoads() {
  memset(queued_private_source_ids_, 0, sizeof(queued_private_source_ids_));
}

bool MCLArrangement::loadQueuedPrivateSource(GridSlot sourceSlot, GridRow row,
                                             EmptyTrack &scratch,
                                             DeviceTrack **out) {
  if (out != nullptr) {
    *out = nullptr;
  }
  if (row != LOAD_QUEUE_PRIVATE_ROW) {
    return false;
  }
  if (out == nullptr || sourceSlot >= NUM_SLOTS) {
    return true;
  }

  GridColumn localCol = 0;
  GridRow localRow = 0;
  uint32_t sourceId = queued_private_source_ids_[sourceSlot];
  if (!privateSourceCell(sourceId, &localCol, &localRow)) {
    return true;
  }

  Grid privateGrid;
  if (!openPrivateGrid(privateGrid, false)) {
    return true;
  }
  *out = scratch.load_from_grid_512(localCol, localRow, &privateGrid);
  privateGrid.close_file();
  return true;
}

#endif  // MCL_FEATURE_HOST_ARRANGER
