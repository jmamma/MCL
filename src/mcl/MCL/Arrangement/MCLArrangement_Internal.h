#pragma once

#include "MCLPlatformFeatures.h"

#if MCL_FEATURE_HOST_ARRANGER

#include "Arrangement/MCLArrangement.h"

#include "DeviceTrack.h"
#include "EmptyTrack.h"
#include "Grid.h"
#include "GridTask.h"
#include "MCLSd.h"
#include "Sequencer/MCLSeq.h"
#include "MCLSysConfig.h"
#include "MidiClock.h"
#include "Project.h"
#include "SeqTrack.h"
#include "Host/SpsHostArrBridge.h"
#include "Host/SpsArrProtocol.h"
#include "TrackLoadFade.h"

#include <string.h>

namespace mcl_arrangement_internal {


constexpr uint16_t kMaxImportClips = 2048;
constexpr uint32_t kMaxPlaybackCatchupQ12 = 64u * 12u;

struct AutomationChunkData {
  mclarrfile::AutomationLane *lanes = nullptr;
  mclarrfile::AutomationPoint *points = nullptr;
  uint16_t lane_count = 0;
  uint32_t point_count = 0;
};

static uint32_t q12ToHostTick96(uint32_t q12) {
  return q12 > 0xFFFFFFFFu / 8u ? 0xFFFFFFFFu : q12 * 8u;
}

static bool enterProjectDir() {
  if (!proj.project_loaded || mcl_cfg.project[0] == '\0') {
    return false;
  }
  proj.chdir_projects();
  return SD.chdir(mcl_cfg.project);
}

static bool buildLeaf(uint8_t idx, char *out, size_t outLen) {
  if (out == nullptr || outLen < 8) {
    return false;
  }
  out[0] = (char)('0' + idx / 100);
  out[1] = (char)('0' + (idx / 10) % 10);
  out[2] = (char)('0' + idx % 10);
  out[3] = '.';
  out[4] = 'a';
  out[5] = 'r';
  out[6] = 'r';
  out[7] = '\0';
  return true;
}

static bool buildRelativePath(uint8_t idx, char *out, size_t outLen) {
  char leaf[8];
  return buildLeaf(idx, leaf, sizeof(leaf)) &&
         MCLSd::join_path(out, (uint8_t)outLen, mclarrfile::kDirName, leaf);
}

static bool buildPrivateGridLeaf(uint8_t idx, char *out, size_t outLen) {
  if (!buildLeaf(idx, out, outLen)) {
    return false;
  }
  out[4] = 'l';
  out[5] = 'o';
  out[6] = 'c';
  return true;
}

static bool buildPrivateGridRelativePath(uint8_t idx, char *out, size_t outLen) {
  char leaf[8];
  return buildPrivateGridLeaf(idx, leaf, sizeof(leaf)) &&
         MCLSd::join_path(out, (uint8_t)outLen, mclarrfile::kDirName, leaf);
}

static bool ensureArrangementDir() {
  if (!enterProjectDir()) {
    return false;
  }
  if (SD.exists(mclarrfile::kDirName)) {
    return true;
  }
  return SD.mkdir(mclarrfile::kDirName, true);
}

static bool privateSourceCell(uint32_t sourceId, GridColumn *col, GridRow *row) {
  if (sourceId == 0) {
    return false;
  }
  uint32_t index = sourceId - 1u;
  uint32_t sourceRow = index / GRID_WIDTH;
  if (sourceRow >= GRID_LENGTH) {
    return false;
  }
  if (col != nullptr) {
    *col = (GridColumn)(index % GRID_WIDTH);
  }
  if (row != nullptr) {
    *row = (GridRow)sourceRow;
  }
  return true;
}

static uint32_t nextPrivateSourceId(const mclarrfile::Clip *clips, uint32_t count) {
  uint32_t nextId = 1;
  for (uint32_t i = 0; clips != nullptr && i < count; ++i) {
    if (clips[i].sourceKind != mclarrfile::CLIP_SOURCE_PRIVATE ||
        clips[i].sourceId < nextId) {
      continue;
    }
    nextId = clips[i].sourceId + 1u;
  }
  GridColumn col = 0;
  GridRow row = 0;
  return privateSourceCell(nextId, &col, &row) ? nextId : 0;
}

static bool openPrivateGrid(Grid &grid, bool create) {
  if (!ensureArrangementDir()) {
    return false;
  }

  char path[32];
  if (!buildPrivateGridRelativePath(mcl_cfg.active_arrangement_idx, path,
                                    sizeof(path))) {
    return false;
  }
  if (grid.open_file(path)) {
    return true;
  }
  if (!create) {
    return false;
  }
  if (!grid.new_grid(path, GRID_VERSION, 0)) {
    return false;
  }
  return grid.open_file(path);
}

static uint32_t linkDurationQ12(const GridLink &link) {
  if (link.loops == 0) {
    return 0;
  }
  uint32_t q12 = (uint32_t)link.loops * link.length *
                 SeqTrack::get_speed_multiplier_int(link.speed_value());
  return q12 < 12 ? 48 : q12;
}

struct SourceCell {
  bool active = false;
  bool loadSound = true;
  GridLink link;
  uint32_t durationQ12 = 0;
  bool hasFade = false;
  TrackLoadFadeData fade;
};

static SourceCell readSourceCell(uint8_t track, GridRow row) {
  SourceCell cell;
  if (track >= NUM_SLOTS || row >= GRID_LENGTH) {
    return cell;
  }
  EmptyTrack scratch;
  DeviceTrack *tr = scratch.load_from_grid_512(track, row);
  if (tr == nullptr || !tr->is_active()) {
    return cell;
  }
  cell.active = true;
  cell.loadSound = tr->load_sound();
  cell.link = tr->link;
  cell.durationQ12 = linkDurationQ12(cell.link);
  cell.fade.init();
  if (const TrackLoadFadeData *fade = tr->load_fade_data()) {
    cell.fade = *fade;
    cell.hasFade = fade->enabled();
  }
  return cell;
}

static void clearClipFade(mclarrfile::Clip &clip) {
  clip.flags &= (uint8_t)~mclarrfile::CLIP_FADE_OVERRIDE;
  clip.fadeFlags = 0;
  clip.fadeTarget = TRACK_LOAD_FADE_TARGET_DEFAULT;
  clip.fadeDurationQ12 = 0;
  clip.fadeAmount = 0;
  clip.fadeCurve = 0;
  clip.fadeReserved = 0;
  clip.endFadeFlags = 0;
  clip.endFadeTarget = TRACK_LOAD_FADE_TARGET_DEFAULT;
  clip.endFadeDurationQ12 = 0;
  clip.endFadeAmount = 0;
  clip.endFadeCurve = 0;
  clip.endFadeReserved = 0;
}

static void setClipFade(mclarrfile::Clip &clip, const TrackLoadFadeData &fade,
                 bool fadeOut, bool overrideFade) {
  if (!overrideFade) {
    if (fadeOut) {
      clip.endFadeFlags = 0;
      clip.endFadeTarget = TRACK_LOAD_FADE_TARGET_DEFAULT;
      clip.endFadeDurationQ12 = 0;
      clip.endFadeAmount = 0;
      clip.endFadeCurve = 0;
      clip.endFadeReserved = 0;
    } else {
      clip.fadeFlags = 0;
      clip.fadeTarget = TRACK_LOAD_FADE_TARGET_DEFAULT;
      clip.fadeDurationQ12 = 0;
      clip.fadeAmount = 0;
      clip.fadeCurve = 0;
      clip.fadeReserved = 0;
    }
    if ((clip.fadeFlags & TRACK_LOAD_FADE_FLAG_ENABLED) == 0 &&
        (clip.endFadeFlags & TRACK_LOAD_FADE_FLAG_ENABLED) == 0) {
      clip.flags &= (uint8_t)~mclarrfile::CLIP_FADE_OVERRIDE;
    }
    return;
  }
  clip.flags |= mclarrfile::CLIP_FADE_OVERRIDE;
  if (fadeOut) {
    clip.endFadeFlags = fade.flags | TRACK_LOAD_FADE_FLAG_OUT;
    clip.endFadeTarget = fade.target;
    clip.endFadeDurationQ12 = fade.duration_q12;
    clip.endFadeAmount = fade.amount;
    clip.endFadeCurve = fade.curve;
    clip.endFadeReserved = 0;
  } else {
    clip.fadeFlags = fade.flags & (uint8_t)~TRACK_LOAD_FADE_FLAG_OUT;
    clip.fadeTarget = fade.target;
    clip.fadeDurationQ12 = fade.duration_q12;
    clip.fadeAmount = fade.amount;
    clip.fadeCurve = fade.curve;
    clip.fadeReserved = 0;
  }
}

static TrackLoadFadeData clipFadeData(const mclarrfile::Clip &clip, bool fadeOut) {
  TrackLoadFadeData fade;
  fade.init();
  if (fadeOut) {
    fade.flags = clip.endFadeFlags | TRACK_LOAD_FADE_FLAG_OUT;
    fade.target = clip.endFadeTarget;
    fade.duration_q12 = clip.endFadeDurationQ12;
    fade.amount = clip.endFadeAmount;
    fade.curve = clip.endFadeCurve;
  } else {
    fade.flags = clip.fadeFlags & (uint8_t)~TRACK_LOAD_FADE_FLAG_OUT;
    fade.target = clip.fadeTarget;
    fade.duration_q12 = clip.fadeDurationQ12;
    fade.amount = clip.fadeAmount;
    fade.curve = clip.fadeCurve;
  }
  fade.reserved[0] = 0;
  fade.reserved[1] = 0;
  return fade;
}

static bool clipFadeAtPosition(const mclarrfile::Clip &clip, uint32_t positionQ12,
                        bool fadeOut, TrackLoadFadeData &fade) {
  if ((clip.flags & mclarrfile::CLIP_FADE_OVERRIDE) == 0) {
    return false;
  }

  fade = clipFadeData(clip, fadeOut);
  if (!fade.enabled() || clip.durationQ12 == 0) {
    return false;
  }

  uint64_t clipStart = clip.startQ12;
  uint64_t clipEnd = clipStart + clip.durationQ12;
  if (clipEnd < clipStart) {
    clipEnd = 0xFFFFFFFFull;
  }
  if ((uint64_t)positionQ12 < clipStart ||
      (uint64_t)positionQ12 >= clipEnd) {
    return false;
  }

  uint64_t fadeDuration =
      fade.duration_q12 < clip.durationQ12 ? fade.duration_q12
                                           : clip.durationQ12;
  if (fadeDuration == 0) {
    return false;
  }

  uint64_t fadeStart = clipStart;
  uint64_t fadeEnd = clipStart + fadeDuration;
  if (fadeOut) {
    fadeEnd = clipEnd;
    fadeStart = clipEnd > fadeDuration ? clipEnd - fadeDuration : clipStart;
  }

  if ((uint64_t)positionQ12 < fadeStart ||
      (uint64_t)positionQ12 >= fadeEnd) {
    return false;
  }

  uint64_t elapsed = (uint64_t)positionQ12 - fadeStart;
  if (elapsed > 0xFFFFu) {
    elapsed = 0xFFFFu;
  }
  fade.set_elapsed_q12((uint16_t)elapsed);
  return true;
}

static bool clipFadeAtPosition(const mclarrfile::Clip &clip, uint32_t positionQ12,
                        TrackLoadFadeData &fade) {
  if (clipFadeAtPosition(clip, positionQ12, true, fade)) {
    return true;
  }
  return clipFadeAtPosition(clip, positionQ12, false, fade);
}

static bool readClipRecord(File &file, const mclarrfile::Header &header,
                    mclarrfile::Clip &clip) {
  memset(&clip, 0, sizeof(clip));
  clearClipFade(clip);
  if (header.clipBytes != sizeof(mclarrfile::Clip) &&
      header.clipBytes != mclarrfile::kClipBytesV4 &&
      header.clipBytes != mclarrfile::kClipBytesV2 &&
      header.clipBytes != mclarrfile::kClipBytesV1) {
    return false;
  }
  bool ok = mcl_sd.read_data(&clip, header.clipBytes, &file);
  if (ok) {
    if (header.clipBytes == mclarrfile::kClipBytesV1) {
      clip.flags &= (uint8_t)~mclarrfile::CLIP_FADE_OVERRIDE;
    } else if (header.clipBytes == mclarrfile::kClipBytesV2 &&
               (clip.fadeFlags & TRACK_LOAD_FADE_FLAG_OUT) != 0) {
      clip.endFadeFlags = clip.fadeFlags;
      clip.endFadeTarget = clip.fadeTarget;
      clip.endFadeDurationQ12 = clip.fadeDurationQ12;
      clip.endFadeAmount = clip.fadeAmount;
      clip.endFadeCurve = clip.fadeCurve;
      clip.endFadeReserved = clip.fadeReserved;
      clip.fadeFlags = 0;
      clip.fadeTarget = TRACK_LOAD_FADE_TARGET_DEFAULT;
      clip.fadeDurationQ12 = 0;
      clip.fadeAmount = 0;
      clip.fadeCurve = 0;
      clip.fadeReserved = 0;
    }
    if (header.clipBytes < mclarrfile::kClipBytesV5) {
      mclarrfile::initGridSource(clip, clip.track);
    } else if (clip.sourceKind != mclarrfile::CLIP_SOURCE_PRIVATE) {
      mclarrfile::initGridSource(clip, mclarrfile::clipSourceTrack(clip));
    }
  }
  return ok;
}

static GridRow activeRowOrZero() {
  return grid_task.last_active_row < GRID_LENGTH ? grid_task.last_active_row : 0;
}

static GridRow resolveTrackBaseRow(uint8_t track, uint8_t requestedStartRow) {
  if (requestedStartRow < GRID_LENGTH &&
      readSourceCell(track, requestedStartRow).active) {
    return requestedStartRow;
  }

  GridRow activeRow = activeRowOrZero();
  if (readSourceCell(track, activeRow).active) {
    return activeRow;
  }

  for (GridRow row = 0; row < GRID_LENGTH; ++row) {
    SourceCell cell = readSourceCell(track, row);
    if (cell.active && cell.durationQ12 != 0) {
      return row;
    }
  }

  for (GridRow row = 0; row < GRID_LENGTH; ++row) {
    if (readSourceCell(track, row).active) {
      return row;
    }
  }

  return activeRow;
}

static bool appendImportLane(uint8_t track, GridRow baseRow, mclarrfile::Clip *clips,
                      uint32_t &count, uint32_t maxClips) {
  int8_t seen[GRID_LENGTH];
  memset(seen, -1, sizeof(seen));

  GridRow row = baseRow;
  uint32_t cursor = 0;
  for (uint16_t guard = 0; guard < GRID_LENGTH && count < maxClips; ++guard) {
    if (seen[row] >= 0) {
      return true;
    }

    SourceCell cell = readSourceCell(track, row);
    if (!cell.active) {
      return true;
    }

    uint32_t duration = cell.durationQ12 != 0 ? cell.durationQ12 : 16u * 12u;
    mclarrfile::Clip &clip = clips[count++];
    clip.startQ12 = cursor;
    clip.durationQ12 = duration;
    clip.repeatQ12 = duration;
    clip.track = track;
    clip.row = row;
    clip.flags = cell.loadSound ? mclarrfile::CLIP_LOAD_SOUND : 0;
    clip.reserved = 0;
    mclarrfile::initGridSource(clip, track);
    if (cell.hasFade) {
      setClipFade(clip, cell.fade, cell.fade.fade_out(), true);
    } else {
      clearClipFade(clip);
    }

    if (cell.durationQ12 == 0 || cell.link.row >= GRID_LENGTH) {
      return true;
    }

    seen[row] = (int8_t)guard;
    cursor += duration;
    row = cell.link.row;
  }
  return true;
}

static void sortClips(mclarrfile::Clip *clips, uint32_t count) {
  for (uint32_t i = 1; i < count; ++i) {
    mclarrfile::Clip key = clips[i];
    uint32_t j = i;
    while (j > 0) {
      const mclarrfile::Clip &prev = clips[j - 1];
      bool after = prev.startQ12 < key.startQ12 ||
                   (prev.startQ12 == key.startQ12 && prev.track <= key.track);
      if (after) {
        break;
      }
      clips[j] = prev;
      --j;
    }
    clips[j] = key;
  }
}

static bool clipOverlaps(const mclarrfile::Clip &clip, uint32_t startQ12,
                  uint32_t endQ12) {
  uint64_t clipStart = clip.startQ12;
  uint64_t clipEnd = clipStart + clip.durationQ12;
  if (clipEnd < clipStart) {
    clipEnd = 0xFFFFFFFFull;
  }
  uint64_t queryEnd = endQ12;
  if (endQ12 <= startQ12) {
    queryEnd = 0xFFFFFFFFull;
  }
  return clipEnd > startQ12 && clipStart < queryEnd;
}

static bool q12InRange(uint32_t value, uint32_t startQ12, uint32_t endQ12) {
  if (endQ12 > startQ12) {
    return value >= startQ12 && value < endQ12;
  }
  return value >= startQ12 || value < endQ12;
}

class ArrangerLoadGroups {
public:
  ArrangerLoadGroups() { init(); }

  void init() {
    count_ = 0;
    memset(delta_, 0, sizeof(delta_));
    memset(mask_, 0, sizeof(mask_));
    memset(privateIds_, 0, sizeof(privateIds_));
    for (uint8_t g = 0; g < NUM_SLOTS; ++g) {
      for (uint8_t t = 0; t < NUM_SLOTS; ++t) {
        rows_[g][t] = 255;
      }
    }
  }

  bool add(GridSlot dst, GridSlot src, GridRow row) {
    if (dst >= NUM_SLOTS || src >= NUM_SLOTS || row >= GRID_LENGTH) {
      return false;
    }
    int8_t d = (int8_t)((int)dst - (int)src);
    uint8_t group = 0xFF;
    for (uint8_t g = 0; g < count_; ++g) {
      if (delta_[g] == d) {
        group = g;
        break;
      }
    }
    if (group == 0xFF) {
      if (count_ >= NUM_SLOTS) {
        return false;
      }
      group = count_++;
      delta_[group] = d;
    }
    mask_[group] |= (uint32_t)(1ul << src);
    rows_[group][src] = row;
    return true;
  }

  bool addPrivate(GridSlot dst, uint32_t sourceId) {
    if (dst >= NUM_SLOTS ||
        !privateSourceCell(sourceId, nullptr, nullptr)) {
      return false;
    }
    uint8_t group = 0xFF;
    for (uint8_t g = 0; g < count_; ++g) {
      if (delta_[g] == 0) {
        group = g;
        break;
      }
    }
    if (group == 0xFF) {
      if (count_ >= NUM_SLOTS) {
        return false;
      }
      group = count_++;
      delta_[group] = 0;
    }
    mask_[group] |= (uint32_t)(1ul << dst);
    rows_[group][dst] = LOAD_QUEUE_PRIVATE_ROW;
    privateIds_[group][dst] = sourceId;
    return true;
  }

  bool flush(uint8_t queueMode) {
    bool queued = false;
    for (uint8_t g = 0; g < count_; ++g) {
      if (mask_[g] == 0) {
        continue;
      }
      GridSlot loadOffset = 255;
      if (delta_[g] != 0) {
        GridSlot firstSource = 255;
        for (uint8_t src = 0; src < NUM_SLOTS; ++src) {
          if ((mask_[g] & (uint32_t)(1ul << src)) != 0) {
            firstSource = src;
            break;
          }
        }
        if (firstSource == 255) {
          continue;
        }
        int firstDest = (int)firstSource + delta_[g];
        if (firstDest < 0 || firstDest >= (int)NUM_SLOTS) {
          continue;
        }
        loadOffset = (GridSlot)firstDest;
      }
      grid_task.load_queue.put_arrangement(queueMode, rows_[g],
                                           privateIds_[g], loadOffset);
      queued = true;
    }
    return queued;
  }

private:
  int8_t delta_[NUM_SLOTS];
  uint32_t mask_[NUM_SLOTS];
  GridRow rows_[NUM_SLOTS][NUM_SLOTS];
  uint32_t privateIds_[NUM_SLOTS][NUM_SLOTS];
  uint8_t count_;
};

static bool addClipLoad(ArrangerLoadGroups &groups, const mclarrfile::Clip &clip) {
  if (clip.sourceKind == mclarrfile::CLIP_SOURCE_PRIVATE) {
    return groups.addPrivate((GridSlot)clip.track, clip.sourceId);
  }
  return groups.add((GridSlot)clip.track,
                    (GridSlot)mclarrfile::clipSourceSlot(clip),
                    (GridRow)clip.row);
}

static bool markerInRange(const mclarrfile::Marker &marker, uint32_t startQ12,
                   uint32_t endQ12) {
  return q12InRange(marker.startQ12, startQ12, endQ12);
}

static bool loopRegionInRange(const mclarrfile::LoopRegion &region,
                       uint32_t startQ12, uint32_t endQ12) {
  if (region.endQ12 <= region.startQ12) {
    return false;
  }
  return region.endQ12 > startQ12 &&
         region.startQ12 < (endQ12 > startQ12 ? endQ12 : 0xFFFFFFFFu);
}

static void sortMarkers(mclarrfile::Marker *markers, uint16_t count) {
  for (uint16_t i = 1; i < count; ++i) {
    mclarrfile::Marker key = markers[i];
    uint16_t j = i;
    while (j > 0) {
      const mclarrfile::Marker &prev = markers[j - 1];
      bool after = prev.startQ12 < key.startQ12 ||
                   (prev.startQ12 == key.startQ12 && prev.track <= key.track);
      if (after) {
        break;
      }
      markers[j] = prev;
      --j;
    }
    markers[j] = key;
  }
}

static void sortLoopRegions(mclarrfile::LoopRegion *regions, uint16_t count) {
  for (uint16_t i = 1; i < count; ++i) {
    mclarrfile::LoopRegion key = regions[i];
    int j = i;
    while (j > 0) {
      const mclarrfile::LoopRegion &prev = regions[j - 1];
      if (prev.startQ12 < key.startQ12 ||
          (prev.startQ12 == key.startQ12 && prev.id <= key.id)) {
        break;
      }
      regions[j] = prev;
      --j;
    }
    regions[j] = key;
  }
}

static void clearTrackLabels(char labels[mclarrfile::kTrackLabelCount]
                                 [mclarrfile::kTrackLabelBytes]) {
  if (labels == nullptr) {
    return;
  }
  memset(labels, 0,
         mclarrfile::kTrackLabelCount * mclarrfile::kTrackLabelBytes);
}

static bool trackLabelsHaveAny(const char labels[mclarrfile::kTrackLabelCount]
                                       [mclarrfile::kTrackLabelBytes]) {
  if (labels == nullptr) {
    return false;
  }
  for (uint8_t track = 0; track < mclarrfile::kTrackLabelCount; ++track) {
    if (labels[track][0] != '\0') {
      return true;
    }
  }
  return false;
}

static void sanitizeLabel(const char *src, char *dst, uint8_t len) {
  if (dst == nullptr || len == 0) {
    return;
  }
  for (uint8_t i = 0; i < len; ++i) {
    dst[i] = '\0';
  }
  if (src == nullptr) {
    return;
  }
  uint8_t i = 0;
  while (i + 1 < len && src[i] != '\0') {
    unsigned char c = (unsigned char)src[i];
    dst[i] = (c >= 32 && c <= 126) ? (char)c : ' ';
    ++i;
  }
}

static bool readHeaderExtra(File &file, const mclarrfile::Header &header,
                     mclarrfile::HeaderExtraV6 *extra) {
  if (extra == nullptr) {
    return false;
  }
  memset(extra, 0, sizeof(*extra));
  if (header.headerBytes < sizeof(mclarrfile::Header) +
                               sizeof(mclarrfile::HeaderExtraV2)) {
    return true;
  }
  if (!file.seekSet(sizeof(mclarrfile::Header))) {
    return false;
  }
  if (header.headerBytes >= sizeof(mclarrfile::Header) +
                              sizeof(mclarrfile::HeaderExtraV6)) {
    return mcl_sd.read_data(extra, sizeof(*extra), &file);
  }
  mclarrfile::HeaderExtraV2 extraV2;
  if (!mcl_sd.read_data(&extraV2, sizeof(extraV2), &file)) {
    return false;
  }
  extra->markerBytes = extraV2.markerBytes;
  extra->markerCount = extraV2.markerCount;
  extra->trackLabelBytes = extraV2.trackLabelBytes;
  extra->trackLabelCount = extraV2.trackLabelCount;
  return true;
}

static bool readHeaderExtraV7(File &file, const mclarrfile::Header &header,
                       mclarrfile::HeaderExtraV7 *extra) {
  if (extra == nullptr) {
    return false;
  }
  memset(extra, 0, sizeof(*extra));
  if (header.version < 7 ||
      header.headerBytes < sizeof(mclarrfile::Header) +
                               sizeof(mclarrfile::HeaderExtraV7)) {
    return true;
  }
  if (!file.seekSet(sizeof(mclarrfile::Header))) {
    return false;
  }
  return mcl_sd.read_data(extra, sizeof(*extra), &file);
}

static bool findChunk(File &file, const mclarrfile::Header &header,
               uint32_t id, mclarrfile::ChunkDirEntry *out) {
  if (out != nullptr) {
    memset(out, 0, sizeof(*out));
  }
  if (header.version < 7 ||
      (header.flags & mclarrfile::HEADER_HAS_CHUNKS) == 0) {
    return false;
  }

  mclarrfile::HeaderExtraV7 extra;
  if (!readHeaderExtraV7(file, header, &extra) ||
      extra.chunkDirBytes != sizeof(mclarrfile::ChunkDirEntry) ||
      extra.chunkCount == 0) {
    return false;
  }

  uint32_t dirOffset =
      sizeof(mclarrfile::Header) + sizeof(mclarrfile::HeaderExtraV7);
  if (dirOffset + (uint32_t)extra.chunkCount * extra.chunkDirBytes >
      header.headerBytes) {
    return false;
  }
  if (!file.seekSet(dirOffset)) {
    return false;
  }

  for (uint16_t i = 0; i < extra.chunkCount; ++i) {
    mclarrfile::ChunkDirEntry entry;
    if (!mcl_sd.read_data(&entry, sizeof(entry), &file)) {
      return false;
    }
    if (entry.id != id) {
      continue;
    }
    if (out != nullptr) {
      *out = entry;
    }
    return true;
  }
  return false;
}

static bool readAutomationData(const mclarrfile::Header &header,
                        AutomationChunkData *automation) {
  if (automation == nullptr) {
    return false;
  }
  automation->lane_count = 0;
  automation->point_count = 0;
  if ((header.flags & mclarrfile::HEADER_HAS_AUTOMATION) == 0 ||
      header.version < 7) {
    return true;
  }

  File file;
  if (!enterProjectDir()) {
    return false;
  }
  char path[16];
  if (!buildRelativePath(mcl_cfg.active_arrangement_idx, path, sizeof(path)) ||
      !file.open(path, O_READ)) {
    return false;
  }

  mclarrfile::ChunkDirEntry laneChunk;
  mclarrfile::ChunkDirEntry pointChunk;
  bool haveLanes = findChunk(file, header, mclarrfile::CHUNK_AUTOMATION_LANES,
                             &laneChunk);
  bool havePoints = findChunk(file, header, mclarrfile::CHUNK_AUTOMATION_POINTS,
                              &pointChunk);

  bool ok = true;
  if (haveLanes) {
    ok = laneChunk.itemBytes == sizeof(mclarrfile::AutomationLane) &&
         laneChunk.count <= mclarrfile::kMaxAutomationLanes &&
         automation->lanes != nullptr && file.seekSet(laneChunk.offset);
    for (uint32_t i = 0; ok && i < laneChunk.count; ++i) {
      ok = mcl_sd.read_data(&automation->lanes[i],
                            sizeof(mclarrfile::AutomationLane), &file);
    }
    if (ok) {
      automation->lane_count = (uint16_t)laneChunk.count;
    }
  }

  if (ok && havePoints) {
    ok = pointChunk.itemBytes == sizeof(mclarrfile::AutomationPoint) &&
         pointChunk.count <= mclarrfile::kMaxAutomationPoints &&
         automation->points != nullptr && file.seekSet(pointChunk.offset);
    for (uint32_t i = 0; ok && i < pointChunk.count; ++i) {
      ok = mcl_sd.read_data(&automation->points[i],
                            sizeof(mclarrfile::AutomationPoint), &file);
    }
    if (ok) {
      automation->point_count = pointChunk.count;
    }
  }

  file.close();
  return ok;
}

static bool readActiveData(
    const mclarrfile::Header &header, mclarrfile::Clip *clips,
    uint32_t *clipCount, mclarrfile::Marker *markers, uint16_t *markerCount,
    char trackLabels[mclarrfile::kTrackLabelCount]
                    [mclarrfile::kTrackLabelBytes],
    mclarrfile::LoopRegion *loopRegions = nullptr,
    uint16_t *loopRegionCount = nullptr) {
  if (clipCount != nullptr) {
    *clipCount = header.clipCount;
  }
  if (markerCount != nullptr) {
    *markerCount = 0;
  }
  if (loopRegionCount != nullptr) {
    *loopRegionCount = 0;
  }
  clearTrackLabels(trackLabels);

  if (header.clipCount > kMaxImportClips) {
    return false;
  }

  File file;
  if (!enterProjectDir()) {
    return false;
  }
  char path[16];
  if (!buildRelativePath(mcl_cfg.active_arrangement_idx, path, sizeof(path)) ||
      !file.open(path, O_READ)) {
    return false;
  }

  bool ok = true;
  if (clips != nullptr && header.clipCount > 0) {
    ok = file.seekSet(header.headerBytes);
    for (uint32_t i = 0; ok && i < header.clipCount; ++i) {
      ok = readClipRecord(file, header, clips[i]);
    }
  }

  mclarrfile::HeaderExtraV6 extra;
  ok = ok && readHeaderExtra(file, header, &extra);
  uint16_t readMarkerCount = 0;
  if (ok && (header.flags & mclarrfile::HEADER_HAS_MARKERS) != 0 &&
      extra.markerBytes == sizeof(mclarrfile::Marker) &&
      extra.markerCount <= mclarrfile::kMaxMarkers) {
    uint32_t markerOffset = header.headerBytes +
                            header.clipCount * header.clipBytes;
    ok = file.seekSet(markerOffset);
    readMarkerCount = extra.markerCount;
    for (uint16_t i = 0; ok && i < readMarkerCount; ++i) {
      mclarrfile::Marker tmp;
      ok = mcl_sd.read_data(&tmp, sizeof(tmp), &file);
      if (ok && markers != nullptr) {
        markers[i] = tmp;
      }
    }
  }
  if (markerCount != nullptr) {
    *markerCount = readMarkerCount;
  }

  if (ok && trackLabels != nullptr &&
      (header.flags & mclarrfile::HEADER_HAS_TRACK_LABELS) != 0 &&
      extra.trackLabelBytes == mclarrfile::kTrackLabelBytes &&
      extra.trackLabelCount <= mclarrfile::kTrackLabelCount) {
    uint32_t labelOffset =
        header.headerBytes + header.clipCount * header.clipBytes +
        (uint32_t)readMarkerCount * extra.markerBytes;
    ok = file.seekSet(labelOffset);
    for (uint16_t i = 0; ok && i < extra.trackLabelCount; ++i) {
      ok = mcl_sd.read_data(trackLabels[i], mclarrfile::kTrackLabelBytes,
                            &file);
    }
  }

  uint16_t readLoopRegionCount = 0;
  if (ok && (header.flags & mclarrfile::HEADER_HAS_LOOP_REGIONS) != 0 &&
      extra.loopRegionBytes == sizeof(mclarrfile::LoopRegion) &&
      extra.loopRegionCount <= mclarrfile::kMaxLoopRegions) {
    uint32_t loopOffset =
        header.headerBytes + header.clipCount * header.clipBytes +
        (uint32_t)readMarkerCount * extra.markerBytes +
        ((header.flags & mclarrfile::HEADER_HAS_TRACK_LABELS) != 0
             ? (uint32_t)extra.trackLabelCount * extra.trackLabelBytes
             : 0u);
    ok = file.seekSet(loopOffset);
    readLoopRegionCount = extra.loopRegionCount;
    for (uint16_t i = 0; ok && i < readLoopRegionCount; ++i) {
      mclarrfile::LoopRegion tmp;
      ok = mcl_sd.read_data(&tmp, sizeof(tmp), &file);
      if (ok && loopRegions != nullptr) {
        loopRegions[i] = tmp;
      }
    }
  }
  if (loopRegionCount != nullptr) {
    *loopRegionCount = readLoopRegionCount;
  }

  file.close();
  return ok;
}

static uint32_t currentClockQ12() {
  uint32_t ticksPer16th = MidiClock.div192th_ticks_per_16th();
  if (ticksPer16th == 0) {
    ticksPer16th = 12;
  }
  const uint32_t div192 = MidiClock.div192th_counter;
  return (div192 / ticksPer16th) * 12u +
         ((div192 % ticksPer16th) * 12u) / ticksPer16th;
}


}  // namespace mcl_arrangement_internal

#endif  // MCL_FEATURE_HOST_ARRANGER
