#include "MCLPlatformFeatures.h"

#if MCL_FEATURE_HOST_ARRANGER

#include "Arrangement/MCLArrangement.h"
#include "MCLArrangement_Internal.h"
#include "ExtTrack.h"
#include "MCLActions.h"
#include "MDSeqTrack.h"
#include "MDTrack.h"
#include "MidiSeqTrack.h"
#include "MidiTrack.h"
#include "SPSXSeqTrack.h"
#include "SPSXTrack.h"

using namespace mcl_arrangement_internal;

namespace {

uint8_t previewLength(uint8_t length) {
  if (length == 0 || length == 0xFF) {
    return 16;
  }
  return length > 64 ? 64 : length;
}

uint8_t previewSpeed(uint8_t speed, const GridLink &link) {
  if (speed == 0xFF) {
    return link.speed_value();
  }
  return speed;
}

uint64_t mdPreviewTrigMask(const MDSeqTrackData &seq) {
  uint64_t mask = 0;
  for (uint8_t step = 0; step < NUM_MD_STEPS && step < 64; step++) {
    if (seq.steps[step].trig) {
      mask |= (1ULL << step);
    }
  }
  return mask;
}

#if !defined(__AVR__)
uint64_t midiPreviewTrigMask(const MidiSeqTrackData &seq) {
  uint64_t mask = 0;
  uint8_t length = previewLength(seq.length);
  uint16_t eventCount = seq.used_event_count();
  for (uint8_t step = 0; step < length && step < 64; step++) {
    uint16_t start = 0;
    uint16_t end = 0;
    seq.locate(step, start, end);
    for (uint16_t idx = start; idx < end && idx < eventCount; idx++) {
      const MidiSeqEvent &event = seq.events[idx];
      if (event.type == MIDI_SEQ_EVENT_NOTE_ON) {
        mask |= (1ULL << step);
        break;
      }
    }
  }
  return mask;
}

uint64_t extPreviewTrigMask(ExtSeqTrackData &seq) {
  uint64_t mask = 0;
  for (uint8_t step = 0; step < NUM_EXT_STEPS && step < 64; step++) {
    if (seq.event_buckets.get(step) != 0) {
      mask |= (1ULL << step);
    }
  }
  return mask;
}
#endif

bool copyLiveSeqToPrivateTrack(DeviceTrack *track, GridDeviceTrack *gdt,
                               uint8_t trackNumber) {
  if (track == nullptr || gdt == nullptr || gdt->seq_track == nullptr) {
    return false;
  }

  track->link.length = gdt->seq_track->length;
  track->link.set_speed(gdt->seq_track->speed);

  switch (gdt->track_type) {
  case MDSPSX_TRACK_TYPE: {
    auto *spsxTrack = track->as<SPSXTrack>();
    if (spsxTrack == nullptr) {
      return false;
    }
    SeqTrack::store_mod_data(spsxTrack->seq_storage.mod(), true,
                             trackNumber);
    if (mcl_seq.using_spsx_tracks) {
      auto *seqTrack = static_cast<SPSXSeqTrack *>(gdt->seq_track);
      spsxTrack->seq_storage.seq_version = SPSX_SEQ_VERSION_SPSX;
      memcpy(spsxTrack->seq_storage.seq_data.spsx.data(),
             seqTrack->SPSXSeqTrackData::data(), sizeof(SPSXSeqTrackData));
    } else {
      auto *seqTrack = static_cast<MDSeqTrack *>(gdt->seq_track);
      spsxTrack->seq_storage.seq_version = SPSX_SEQ_VERSION_LEGACY;
      memcpy(spsxTrack->seq_storage.seq_data.legacy.data(), seqTrack->data(),
             sizeof(MDSeqTrackData));
    }
    return true;
  }
  case MD_TRACK_TYPE: {
    auto *mdTrack = track->as<MDTrack>();
    if (mdTrack == nullptr) {
      return false;
    }
    mcl_seq.md_arp_tracks[trackNumber].store_data(&mdTrack->mod_data.arp);
    mcl_seq.md_arp_tracks[trackNumber].store_phase_data(
        mdTrack->mod_data.arp_phase());
    mcl_seq.grid_x_lfo_tracks[trackNumber].store_data(&mdTrack->mod_data.lfo);
    auto *seqTrack = static_cast<MDSeqTrack *>(gdt->seq_track);
    memcpy(mdTrack->seq_data.data(), seqTrack->data(), sizeof(MDSeqTrackData));
    return true;
  }
  case MIDI_TRACK_TYPE: {
    auto *midiTrack = track->as<MidiTrack>();
    if (midiTrack == nullptr) {
      return false;
    }
    auto *seqTrack = static_cast<MidiSeqTrack *>(gdt->seq_track);
    SeqTrack::store_mod_data(midiTrack->seq_data.mod(), false, trackNumber);
    static_cast<MidiSeqTrackData &>(midiTrack->seq_data) = seqTrack->seq_data;
    midiTrack->seq_data.channel = seqTrack->channel();
    midiTrack->seq_data.length = gdt->seq_track->length;
    midiTrack->seq_data.speed = gdt->seq_track->speed;
    return true;
  }
  case EXT_TRACK_TYPE: {
    auto *extTrack = track->as<ExtTrack>();
    if (extTrack == nullptr) {
      return false;
    }
    auto *seqTrack = static_cast<ExtSeqTrack *>(gdt->seq_track);
    SeqTrack::store_mod_data(extTrack->mod_data, false, trackNumber);
    memcpy(&extTrack->seq_data, seqTrack->data(), sizeof(extTrack->seq_data));
    return true;
  }
  default:
    return false;
  }
}

} // namespace

bool MCLArrangement::createPrivateSourceFromGrid(GridSlot sourceSlot,
                                                 GridRow row,
                                                 GridSlot dstTrack,
                                                 uint32_t *sourceIdOut) {
  if (sourceIdOut != nullptr) {
    *sourceIdOut = 0;
  }
  if (sourceSlot >= NUM_SLOTS || dstTrack >= NUM_SLOTS || row >= GRID_LENGTH ||
      !ensureActive()) {
    return false;
  }

  EmptyTrack scratch;
  DeviceTrack *source = scratch.load_from_grid_512(sourceSlot, row);
  if (source == nullptr || !source->is_active()) {
    return false;
  }

  Grid privateGrid;
  if (!openPrivateGrid(privateGrid, true)) {
    return false;
  }

  uint32_t sourceId = 0;
  GridColumn localCol = 0;
  GridRow localRow = 0;
  uint32_t maxSourceId = (uint32_t)GRID_WIDTH * (uint32_t)GRID_LENGTH;
  for (uint32_t candidate = 1; candidate <= maxSourceId; ++candidate) {
    GridColumn col = 0;
    GridRow privateRow = 0;
    if (!privateSourceCell(candidate, &col, &privateRow)) {
      break;
    }
    EmptyTrack existingScratch;
    DeviceTrack *existing =
        existingScratch.load_from_grid_512(col, privateRow, &privateGrid);
    if (existing == nullptr || !existing->is_active()) {
      sourceId = candidate;
      localCol = col;
      localRow = privateRow;
      break;
    }
  }
  if (sourceId == 0) {
    privateGrid.close_file();
    return false;
  }

  source->on_copy(sourceSlot & 0x0F, dstTrack & 0x0F, false);
  bool stored = source->store_in_grid(localCol, localRow, nullptr, 0, false,
                                      &privateGrid);
  stored = stored && privateGrid.sync();
  privateGrid.close_file();
  if (!stored) {
    return false;
  }

  if (sourceIdOut != nullptr) {
    *sourceIdOut = sourceId;
  }
  return true;
}

bool MCLArrangement::privateSourcePreview(uint32_t sourceId,
                                          uint8_t *trackType,
                                          uint8_t *length,
                                          uint8_t *speed,
                                          uint64_t *trigMask) {
  if (trackType != nullptr) {
    *trackType = EMPTY_TRACK_TYPE;
  }
  if (length != nullptr) {
    *length = 16;
  }
  if (speed != nullptr) {
    *speed = SEQ_SPEED_1X;
  }
  if (trigMask != nullptr) {
    *trigMask = 0;
  }

  GridColumn col = 0;
  GridRow row = 0;
  if (!privateSourceCell(sourceId, &col, &row)) {
    return false;
  }

  Grid privateGrid;
  if (!openPrivateGrid(privateGrid, false)) {
    return false;
  }

  EmptyTrack scratch;
  DeviceTrack *source = scratch.load_from_grid_512(col, row, &privateGrid);
  if (source == nullptr || !source->is_active()) {
    privateGrid.close_file();
    return false;
  }

  uint8_t outType = source->active;
  uint8_t outLength = previewLength(source->link.length);
  uint8_t outSpeed = source->link.speed_value();
  uint64_t outMask = 0;

  switch (source->active) {
  case MDSPSX_TRACK_TYPE: {
    auto *spsxTrack = source->as<SPSXTrack>();
    if (spsxTrack == nullptr) {
      break;
    }
    if (spsxTrack->has_spsx_seq()) {
      const auto &seq = spsxTrack->seq_storage.seq_data.spsx;
      outLength = previewLength(seq.track_length != 0 ? seq.track_length
                                                       : source->link.length);
      outSpeed = previewSpeed(seq.track_speed, source->link);
      outMask = seq.trig_mask;
    } else {
      outMask = mdPreviewTrigMask(spsxTrack->seq_storage.seq_data.legacy);
    }
    break;
  }
  case MD_TRACK_TYPE: {
    auto *mdTrack = source->as<MDTrack>();
    if (mdTrack != nullptr) {
      outMask = mdPreviewTrigMask(mdTrack->seq_data);
    }
    break;
  }
#if !defined(__AVR__)
  case MIDI_TRACK_TYPE: {
    auto *midiTrack = source->as<MidiTrack>();
    if (midiTrack != nullptr) {
      outLength = previewLength(midiTrack->seq_data.length);
      outSpeed = midiTrack->seq_data.speed;
      outMask = midiPreviewTrigMask(midiTrack->seq_data);
    }
    break;
  }
  case EXT_TRACK_TYPE: {
    auto *extTrack = source->as<ExtTrack>();
    if (extTrack != nullptr) {
      outMask = extPreviewTrigMask(extTrack->seq_data);
    }
    break;
  }
#endif
  default:
    break;
  }

  privateGrid.close_file();
  if (trackType != nullptr) {
    *trackType = outType;
  }
  if (length != nullptr) {
    *length = outLength;
  }
  if (speed != nullptr) {
    *speed = outSpeed;
  }
  if (trigMask != nullptr) {
    *trigMask = outMask;
  }
  return true;
}

bool MCLArrangement::makeClipLocal(uint32_t startQ12, uint32_t durationQ12,
                                   uint8_t track, uint8_t row,
                                   GridSlot expectedSourceSlot,
                                   uint32_t *sourceIdOut) {
  if (sourceIdOut != nullptr) {
    *sourceIdOut = 0;
  }
  if (track >= NUM_SLOTS || row >= GRID_LENGTH || durationQ12 == 0 ||
      !ensureActive()) {
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

  mclarrfile::Clip &clip = clips[clipIndex];
  GridSlot sourceSlot = (GridSlot)mclarrfile::clipSourceSlot(clip);
  uint32_t sourceId = 0;
  if (!createPrivateSourceFromGrid(sourceSlot, clip.row, clip.track,
                                   &sourceId)) {
    return false;
  }

  clip.sourceKind = mclarrfile::CLIP_SOURCE_PRIVATE;
  clip.sourceTrack = clip.track;
  clip.sourceFlags = 0;
  clip.sourceReserved = 0;
  clip.sourceId = sourceId;

  bool ok = rewriteActiveWithMetadata(header, clips.get(), clipCount,
                                      markers.get(), markerCount, labels,
                                      loopRegions.get(), loopRegionCount);
  if (ok) {
    if (sourceIdOut != nullptr) {
      *sourceIdOut = sourceId;
    }
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

uint32_t MCLArrangement::runtimePrivateSourceMask() const {
  uint32_t mask = 0;
  for (uint8_t slot = 0; slot < NUM_SLOTS && slot < 32; ++slot) {
    if (runtime_private_source_ids_[slot] != 0) {
      mask |= (uint32_t)(1ul << slot);
    }
  }
  return mask;
}

uint32_t MCLArrangement::runtimePrivateSourceId(uint8_t dst) const {
  if (dst >= NUM_SLOTS) {
    return 0;
  }
  return runtime_private_source_ids_[dst];
}

void MCLArrangement::setRuntimePrivateSource(uint8_t dst, uint32_t sourceId) {
  if (dst >= NUM_SLOTS) {
    return;
  }
  runtime_private_source_ids_[dst] = sourceId;
  if (dst < 32) {
    runtime_private_dirty_mask_ &= ~(uint32_t)(1ul << dst);
  }
}

void MCLArrangement::clearRuntimePrivateSource(uint8_t dst) {
  if (dst >= NUM_SLOTS) {
    return;
  }
  runtime_private_source_ids_[dst] = 0;
  if (dst < 32) {
    runtime_private_dirty_mask_ &= ~(uint32_t)(1ul << dst);
  }
}

void MCLArrangement::clearRuntimePrivateSources(uint32_t mask) {
  for (uint8_t slot = 0; slot < NUM_SLOTS && slot < 32; ++slot) {
    if ((mask & (uint32_t)(1ul << slot)) != 0) {
      runtime_private_source_ids_[slot] = 0;
    }
  }
  runtime_private_dirty_mask_ &= ~mask;
}

bool MCLArrangement::flushRuntimePrivateSource(uint8_t dst) {
  if (dst >= NUM_SLOTS || runtime_private_source_ids_[dst] == 0) {
    return false;
  }

  GridDeviceTrack *gdt = mcl_actions.get_grid_dev_track(dst);
  if (gdt == nullptr || gdt->seq_track == nullptr ||
      gdt->mem_slot_idx >= GRID_WIDTH) {
    return false;
  }

  GridColumn localCol = 0;
  GridRow localRow = 0;
  if (!privateSourceCell(runtime_private_source_ids_[dst], &localCol,
                         &localRow)) {
    return false;
  }

  Grid privateGrid;
  if (!openPrivateGrid(privateGrid, true)) {
    return false;
  }

  GridLink savedLink;
  TrackLoadFadeData savedLoadFade;
  savedLoadFade.init();
  bool haveSavedLink = false;
  EmptyTrack existingScratch;
  DeviceTrack *existing =
      existingScratch.load_from_grid_512(localCol, localRow, &privateGrid);
  if (existing != nullptr && existing->is_active()) {
    savedLink = existing->link;
    haveSavedLink = true;
    const TrackLoadFadeData *existingFade = existing->load_fade_data();
    if (existingFade != nullptr) {
      savedLoadFade = *existingFade;
    }
  }
  if (!haveSavedLink) {
    savedLink.init(localRow);
  }

  EmptyTrack liveScratch;
  DeviceTrack *live =
      liveScratch.load_from_mem(gdt->mem_slot_idx, gdt->track_type);
  if (live == nullptr) {
    live = liveScratch.init_track_type(gdt->track_type);
    if (live == nullptr) {
      privateGrid.close_file();
      return false;
    }
    live->init((uint8_t)(dst & 0x0F), gdt->seq_track);
  }
  live->link = savedLink;
  TrackLoadFadeData *liveFade = live->load_fade_data();
  if (liveFade != nullptr) {
    *liveFade = savedLoadFade;
  }

  uint8_t trackNumber = (uint8_t)(dst & 0x0F);
  bool copiedSeq = copyLiveSeqToPrivateTrack(live, gdt, trackNumber);
  bool stored = copiedSeq
                    ? live->write_grid(live->_this(), live->get_store_size(),
                                       localCol, localRow, &privateGrid)
                    : live->store_in_grid(localCol, localRow, gdt->seq_track,
                                          0, false, &privateGrid);
  stored = stored && privateGrid.sync();
  privateGrid.close_file();
  if (stored && dst < 32) {
    runtime_private_dirty_mask_ &= ~(uint32_t)(1ul << dst);
  }
  return stored;
}

bool MCLArrangement::markRuntimePrivateSourceEdited(uint8_t dst) {
  if (dst >= NUM_SLOTS || runtime_private_source_ids_[dst] == 0) {
    return false;
  }
  if (dst < 32) {
    runtime_private_dirty_mask_ |= (uint32_t)(1ul << dst);
  }
  return true;
}

bool MCLArrangement::flushRuntimePrivateSourceEdits() {
  uint32_t mask = runtime_private_dirty_mask_;
  if (mask == 0) {
    return false;
  }

  bool flushed = false;
  for (uint8_t slot = 0; slot < NUM_SLOTS && slot < 32; ++slot) {
    if ((mask & (uint32_t)(1ul << slot)) == 0) {
      continue;
    }
    flushed = flushRuntimePrivateSource(slot) || flushed;
  }
  return flushed;
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
    clearRuntimePrivateSource(sourceSlot);
    return true;
  }

  Grid privateGrid;
  if (!openPrivateGrid(privateGrid, false)) {
    clearRuntimePrivateSource(sourceSlot);
    return true;
  }
  *out = scratch.load_from_grid_512(localCol, localRow, &privateGrid);
  privateGrid.close_file();
  if (*out != nullptr) {
    setRuntimePrivateSource(sourceSlot, sourceId);
  } else {
    clearRuntimePrivateSource(sourceSlot);
  }
  return true;
}

#endif  // MCL_FEATURE_HOST_ARRANGER
