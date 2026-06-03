/**
 * MCLArrangement - project-local arrangement storage.
 */
#if !defined(__AVR__)

#include "MCLArrangement.h"

#include "DeviceTrack.h"
#include "EmptyTrack.h"
#include "GridTask.h"
#include "MCLSd.h"
#include "MCLSysConfig.h"
#include "MidiClock.h"
#include "Project.h"
#include "SeqTrack.h"
#include "SpsArrProtocol.h"
#include "TrackLoadFade.h"

#include <string.h>

MCLArrangement mcl_arrangement;

namespace {

constexpr uint16_t kMaxImportClips = 2048;
constexpr uint32_t kMaxPlaybackCatchupQ12 = 64u * 12u;

bool enterProjectDir() {
  if (!proj.project_loaded || mcl_cfg.project[0] == '\0') {
    return false;
  }
  proj.chdir_projects();
  return SD.chdir(mcl_cfg.project);
}

bool buildLeaf(uint8_t idx, char *out, size_t outLen) {
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

bool buildRelativePath(uint8_t idx, char *out, size_t outLen) {
  char leaf[8];
  return buildLeaf(idx, leaf, sizeof(leaf)) &&
         MCLSd::join_path(out, (uint8_t)outLen, mclarrfile::kDirName, leaf);
}

bool ensureArrangementDir() {
  if (!enterProjectDir()) {
    return false;
  }
  if (SD.exists(mclarrfile::kDirName)) {
    return true;
  }
  return SD.mkdir(mclarrfile::kDirName, true);
}

uint32_t linkDurationQ12(const GridLink &link) {
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

SourceCell readSourceCell(uint8_t track, GridRow row) {
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

void clearClipFade(mclarrfile::Clip &clip) {
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

void setClipFade(mclarrfile::Clip &clip, const TrackLoadFadeData &fade,
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

TrackLoadFadeData clipFadeData(const mclarrfile::Clip &clip, bool fadeOut) {
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

bool clipFadeAtPosition(const mclarrfile::Clip &clip, uint32_t positionQ12,
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

bool clipFadeAtPosition(const mclarrfile::Clip &clip, uint32_t positionQ12,
                        TrackLoadFadeData &fade) {
  if (clipFadeAtPosition(clip, positionQ12, true, fade)) {
    return true;
  }
  return clipFadeAtPosition(clip, positionQ12, false, fade);
}

bool readClipRecord(File &file, const mclarrfile::Header &header,
                    mclarrfile::Clip &clip) {
  memset(&clip, 0, sizeof(clip));
  clearClipFade(clip);
  if (header.clipBytes != sizeof(mclarrfile::Clip) &&
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
  }
  return ok;
}

GridRow activeRowOrZero() {
  return grid_task.last_active_row < GRID_LENGTH ? grid_task.last_active_row : 0;
}

GridRow resolveTrackBaseRow(uint8_t track, uint8_t requestedStartRow) {
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

bool appendImportLane(uint8_t track, GridRow baseRow, mclarrfile::Clip *clips,
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

void sortClips(mclarrfile::Clip *clips, uint32_t count) {
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

bool clipOverlaps(const mclarrfile::Clip &clip, uint32_t startQ12,
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

bool q12InRange(uint32_t value, uint32_t startQ12, uint32_t endQ12) {
  if (endQ12 > startQ12) {
    return value >= startQ12 && value < endQ12;
  }
  return value >= startQ12 || value < endQ12;
}

bool markerInRange(const mclarrfile::Marker &marker, uint32_t startQ12,
                   uint32_t endQ12) {
  return q12InRange(marker.startQ12, startQ12, endQ12);
}

void sortMarkers(mclarrfile::Marker *markers, uint16_t count) {
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

void clearTrackLabels(char labels[mclarrfile::kTrackLabelCount]
                                 [mclarrfile::kTrackLabelBytes]) {
  if (labels == nullptr) {
    return;
  }
  memset(labels, 0,
         mclarrfile::kTrackLabelCount * mclarrfile::kTrackLabelBytes);
}

bool trackLabelsHaveAny(const char labels[mclarrfile::kTrackLabelCount]
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

void sanitizeLabel(const char *src, char *dst, uint8_t len) {
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

bool readHeaderExtra(File &file, const mclarrfile::Header &header,
                     mclarrfile::HeaderExtraV2 *extra) {
  if (extra == nullptr) {
    return false;
  }
  memset(extra, 0, sizeof(*extra));
  if (header.headerBytes < sizeof(mclarrfile::Header) +
                               sizeof(mclarrfile::HeaderExtraV2)) {
    return true;
  }
  return file.seekSet(sizeof(mclarrfile::Header)) &&
         mcl_sd.read_data(extra, sizeof(*extra), &file);
}

bool readActiveData(
    const mclarrfile::Header &header, mclarrfile::Clip *clips,
    uint32_t *clipCount, mclarrfile::Marker *markers, uint16_t *markerCount,
    char trackLabels[mclarrfile::kTrackLabelCount]
                    [mclarrfile::kTrackLabelBytes]) {
  if (clipCount != nullptr) {
    *clipCount = header.clipCount;
  }
  if (markerCount != nullptr) {
    *markerCount = 0;
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

  mclarrfile::HeaderExtraV2 extra;
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

  file.close();
  return ok;
}

uint32_t currentClockQ12() {
  uint32_t ticksPer16th = MidiClock.div192th_ticks_per_16th();
  if (ticksPer16th == 0) {
    ticksPer16th = 12;
  }
  const uint32_t div192 = MidiClock.div192th_counter;
  return (div192 / ticksPer16th) * 12u +
         ((div192 % ticksPer16th) * 12u) / ticksPer16th;
}

} // namespace

bool MCLArrangement::openIndex(File *file, uint8_t idx, uint8_t mode) {
  if (file == nullptr || !enterProjectDir()) {
    return false;
  }
  char path[16];
  if (!buildRelativePath(idx, path, sizeof(path))) {
    return false;
  }
  return file->open(path, mode);
}

bool MCLArrangement::openActive(File *file, uint8_t mode) {
  return openIndex(file, mcl_cfg.active_arrangement_idx, mode);
}

bool MCLArrangement::create(uint8_t idx, const char *name) {
  if (!ensureArrangementDir()) {
    return false;
  }
  char path[16];
  if (!buildRelativePath(idx, path, sizeof(path))) {
    return false;
  }
  File file;
  if (!file.open(path, O_RDWR | O_CREAT)) {
    return false;
  }
  mclarrfile::Header header;
  mclarrfile::initHeader(header, name && name[0] ? name : "arrangement");
  bool ok = mcl_sd.write_data(&header, sizeof(header), &file);
  ok = ok && file.truncate(sizeof(header));
  ok = ok && file.sync();
  file.close();
  return ok;
}

bool MCLArrangement::createFirst(uint8_t *idxOut) {
  if (!ensureArrangementDir()) {
    return false;
  }
  for (uint16_t idx = 0; idx <= mclarrfile::kMaxArrangementIndex; ++idx) {
    char path[16];
    if (!buildRelativePath((uint8_t)idx, path, sizeof(path))) {
      return false;
    }
    if (SD.exists(path)) {
      continue;
    }
    char name[8];
    buildLeaf((uint8_t)idx, name, sizeof(name));
    name[3] = '\0';
    if (!create((uint8_t)idx, name)) {
      return false;
    }
    if (idxOut != nullptr) {
      *idxOut = (uint8_t)idx;
    }
    return true;
  }
  return false;
}

bool MCLArrangement::select(uint8_t idx) {
  if (!ensureActive() && idx == mcl_cfg.active_arrangement_idx) {
    return false;
  }
  File file;
  if (!openIndex(&file, idx, O_READ)) {
    file.close();
    if (!create(idx, "arrangement")) {
      return false;
    }
  } else {
    mclarrfile::Header header;
    bool ok = mcl_sd.read_data(&header, sizeof(header), &file) &&
              mclarrfile::validHeader(header);
    file.close();
    if (!ok) {
      return false;
    }
  }
  mcl_cfg.active_arrangement_idx = idx;
  bool ok = proj.store_config_from_system();
  if (ok) {
    resetPlayback();
  }
  return ok;
}

bool MCLArrangement::ensureActive() {
  File file;
  if (openActive(&file, O_READ)) {
    mclarrfile::Header header;
    bool ok = mcl_sd.read_data(&header, sizeof(header), &file) &&
              mclarrfile::validHeader(header);
    file.close();
    if (ok) {
      return true;
    }
  } else {
    file.close();
  }

  uint8_t idx = mcl_cfg.active_arrangement_idx;
  if (!create(idx, idx == 0 ? "main" : "arrangement")) {
    return false;
  }
  return true;
}

bool MCLArrangement::readMeta(mclarrfile::Header *header) {
  if (header == nullptr || !ensureActive()) {
    return false;
  }
  File file;
  if (!openActive(&file, O_READ)) {
    return false;
  }
  bool ok = mcl_sd.read_data(header, sizeof(*header), &file) &&
            mclarrfile::validHeader(*header);
  file.close();
  return ok;
}

bool MCLArrangement::rewriteActive(const mclarrfile::Header &header,
                                   const mclarrfile::Clip *clips,
                                   uint32_t clipCount) {
  static mclarrfile::Marker markers[mclarrfile::kMaxMarkers];
  char labels[mclarrfile::kTrackLabelCount][mclarrfile::kTrackLabelBytes];
  uint16_t markerCount = 0;
  readActiveData(header, nullptr, nullptr, markers, &markerCount, labels);
  return rewriteActiveWithMetadata(header, clips, clipCount, markers,
                                   markerCount, labels);
}

bool MCLArrangement::rewriteActiveWithMarkers(
    const mclarrfile::Header &header, const mclarrfile::Clip *clips,
    uint32_t clipCount, const mclarrfile::Marker *markers,
    uint16_t markerCount) {
  char labels[mclarrfile::kTrackLabelCount][mclarrfile::kTrackLabelBytes];
  clearTrackLabels(labels);
  readActiveData(header, nullptr, nullptr, nullptr, nullptr, labels);
  return rewriteActiveWithMetadata(header, clips, clipCount, markers,
                                   markerCount, labels);
}

bool MCLArrangement::rewriteActiveWithMetadata(
    const mclarrfile::Header &header, const mclarrfile::Clip *clips,
    uint32_t clipCount, const mclarrfile::Marker *markers,
    uint16_t markerCount,
    const char trackLabels[mclarrfile::kTrackLabelCount]
                          [mclarrfile::kTrackLabelBytes]) {
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
  bool haveExtra = markerCount > 0 || haveLabels;
  outHeader.headerBytes = haveExtra
                              ? (uint16_t)(sizeof(mclarrfile::Header) +
                                           sizeof(mclarrfile::HeaderExtraV2))
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
  bool ok = file.seekSet(0) &&
            mcl_sd.write_data(&outHeader, sizeof(outHeader), &file);
  if (ok && haveExtra) {
    mclarrfile::HeaderExtraV2 extra;
    extra.markerBytes = markerCount > 0 ? sizeof(mclarrfile::Marker) : 0;
    extra.markerCount = markerCount;
    extra.trackLabelBytes =
        haveLabels ? mclarrfile::kTrackLabelBytes : (uint16_t)0;
    extra.trackLabelCount =
        haveLabels ? mclarrfile::kTrackLabelCount : (uint16_t)0;
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
  uint32_t fileSize = sizeof(mclarrfile::Header) +
                      (haveExtra ? sizeof(mclarrfile::HeaderExtraV2) : 0) +
                      clipCount * sizeof(mclarrfile::Clip) +
                      markerCount * sizeof(mclarrfile::Marker) +
                      (haveLabels ? (uint32_t)mclarrfile::kTrackLabelCount *
                                        mclarrfile::kTrackLabelBytes
                                  : 0);
  ok = ok && file.truncate(fileSize);
  ok = ok && file.sync();
  file.close();
  return ok;
}

bool MCLArrangement::clearActive() {
  mclarrfile::Header header;
  if (!readMeta(&header)) {
    mclarrfile::initHeader(header, "main");
  }
  char labels[mclarrfile::kTrackLabelCount][mclarrfile::kTrackLabelBytes];
  clearTrackLabels(labels);
  readActiveData(header, nullptr, nullptr, nullptr, nullptr, labels);
  bool ok = rewriteActiveWithMetadata(header, nullptr, 0, nullptr, 0, labels);
  if (ok) {
    resetPlayback();
  }
  return ok;
}

bool MCLArrangement::saveActive() {
  File file;
  if (!openActive(&file, O_RDWR)) {
    return false;
  }
  bool ok = file.sync();
  file.close();
  return ok;
}

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

  mclarrfile::HeaderExtraV2 extra;
  bool ok = file.seekSet(sizeof(mclarrfile::Header)) &&
            mcl_sd.read_data(&extra, sizeof(extra), &file) &&
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

bool MCLArrangement::setTrackLabel(uint8_t track, const char *label) {
  if (track >= NUM_SLOTS || track >= mclarrfile::kTrackLabelCount) {
    return false;
  }
  if (!ensureActive()) {
    return false;
  }

  static mclarrfile::Clip clips[kMaxImportClips];
  static mclarrfile::Marker markers[mclarrfile::kMaxMarkers];
  char labels[mclarrfile::kTrackLabelCount][mclarrfile::kTrackLabelBytes];
  uint32_t clipCount = 0;
  uint16_t markerCount = 0;

  mclarrfile::Header header;
  if (!readMeta(&header)) {
    return false;
  }
  if (!readActiveData(header, clips, &clipCount, markers, &markerCount,
                      labels)) {
    return false;
  }

  sanitizeLabel(label, labels[track], mclarrfile::kTrackLabelBytes);
  bool ok = rewriteActiveWithMetadata(header, clips, clipCount, markers,
                                      markerCount, labels);
  if (ok) {
    resetPlayback();
  }
  return ok;
}

bool MCLArrangement::setMarkerLabel(uint32_t startQ12, uint8_t track,
                                    const char *label) {
  if (track != spsarr::kArrMarkerGlobalTrack &&
      (track >= NUM_SLOTS || track >= 16)) {
    return false;
  }
  if (!ensureActive()) {
    return false;
  }

  static mclarrfile::Clip clips[kMaxImportClips];
  static mclarrfile::Marker markers[mclarrfile::kMaxMarkers];
  char labels[mclarrfile::kTrackLabelCount][mclarrfile::kTrackLabelBytes];
  uint32_t clipCount = 0;
  uint16_t markerCount = 0;

  mclarrfile::Header header;
  if (!readMeta(&header)) {
    return false;
  }
  if (!readActiveData(header, clips, &clipCount, markers, &markerCount,
                      labels)) {
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
                                      markerCount, labels);
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
  char labels[mclarrfile::kTrackLabelCount][mclarrfile::kTrackLabelBytes];
  uint32_t clipCount = 0;
  uint16_t markerCount = 0;

  mclarrfile::Header header;
  if (!readMeta(&header)) {
    return false;
  }
  if (!readActiveData(header, clips, &clipCount, markers, &markerCount,
                      labels)) {
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
                                      markerCount, labels);
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
  }
  return ok;
}

void MCLArrangement::resetPlayback() {
  playback_arrangement_idx_ = 0xFF;
  last_tick_q12_ = 0;
  playback_active_mask_ = 0;
  clip_runtime_fade_mask_ = 0;
  playback_active_ = false;
}

void MCLArrangement::armClipRuntime(uint8_t dst, const mclarrfile::Clip &clip,
                                    uint16_t elapsedQ12) {
  if (dst >= 16) {
    return;
  }
  uint16_t bit = (uint16_t)(1u << dst);
  if ((clip.flags & mclarrfile::CLIP_FADE_OVERRIDE) == 0) {
    clip_runtime_fade_mask_ &= (uint16_t)~bit;
    return;
  }
  TrackLoadFadeData fade = clipFadeData(clip, false);
  if (!fade.enabled()) {
    fade = clipFadeData(clip, true);
  }
  if (!fade.enabled()) {
    clip_runtime_fade_mask_ &= (uint16_t)~bit;
    return;
  }
  fade.set_elapsed_q12(elapsedQ12);
  clip_runtime_fades_[dst] = fade;
  clip_runtime_fade_mask_ |= bit;
}

bool MCLArrangement::armRuntimeForHostLoad(uint32_t positionQ12,
                                           const GridRow rows[NUM_SLOTS],
                                           uint16_t trackMask,
                                           GridSlot loadOffset) {
  if (rows == nullptr || !ensureActive()) {
    return false;
  }

  GridRow loadedRows[NUM_SLOTS];
  memset(loadedRows, 255, sizeof(loadedRows));
  uint16_t loadMask = 0;
  uint16_t clearMask = 0;

  GridSlot firstSource = 255;
  for (uint8_t src = 0; src < NUM_SLOTS && src < 16; ++src) {
    if (((trackMask >> src) & 1u) == 0 || rows[src] >= GRID_LENGTH) {
      continue;
    }
    firstSource = src;
    break;
  }

  for (uint8_t src = 0; src < NUM_SLOTS && src < 16; ++src) {
    if (((trackMask >> src) & 1u) == 0) {
      continue;
    }
    if (rows[src] < GRID_LENGTH) {
      GridSlot dst = src;
      if (loadOffset < NUM_SLOTS) {
        if (firstSource == 255) {
          continue;
        }
        int mapped = (int)src - (int)firstSource + (int)loadOffset;
        if (mapped < 0 || mapped >= (int)NUM_SLOTS) {
          continue;
        }
        dst = (GridSlot)mapped;
      }
      if (dst < 16) {
        loadedRows[dst] = rows[src];
        loadMask |= (uint16_t)(1u << dst);
      }
      continue;
    }
    if (rows[src] == LOAD_QUEUE_CLEAR_ROW) {
      clearMask |= (uint16_t)(1u << src);
    }
  }

  const uint16_t touchedMask = (uint16_t)(loadMask | clearMask);
  clip_runtime_fade_mask_ &= (uint16_t)~touchedMask;
  if (touchedMask == 0) {
    return false;
  }

  File file;
  if (!openActive(&file, O_READ)) {
    return false;
  }

  mclarrfile::Header header;
  bool ok = mcl_sd.read_data(&header, sizeof(header), &file) &&
            mclarrfile::validHeader(header) &&
            file.seekSet(header.headerBytes);
  if (!ok) {
    file.close();
    return false;
  }

  bool armed = false;
  for (uint32_t i = 0; i < header.clipCount; ++i) {
    mclarrfile::Clip clip;
    if (!readClipRecord(file, header, clip)) {
      break;
    }
    if ((clip.flags & mclarrfile::CLIP_MUTED) != 0 ||
        clip.durationQ12 == 0 || clip.track >= 16 ||
        clip.row >= GRID_LENGTH) {
      continue;
    }

    uint64_t clipStart = clip.startQ12;
    uint64_t clipEnd = clipStart + clip.durationQ12;
    if (clipEnd < clipStart) {
      clipEnd = 0xFFFFFFFFull;
    }

    const uint16_t bit = (uint16_t)(1u << clip.track);
    if ((loadMask & bit) != 0 && loadedRows[clip.track] == clip.row &&
        (uint64_t)positionQ12 >= clipStart &&
        (uint64_t)positionQ12 < clipEnd) {
      TrackLoadFadeData fade;
      if (clipFadeAtPosition(clip, positionQ12, fade)) {
        armRuntimeFade(clip.track, fade);
        armed = true;
      }
      continue;
    }

    if ((clearMask & bit) != 0 && positionQ12 > 0) {
      uint64_t previous = (uint64_t)positionQ12 - 1u;
      TrackLoadFadeData fade;
      if (previous >= clipStart && previous < clipEnd &&
          clipFadeAtPosition(clip, (uint32_t)previous, true, fade)) {
        armRuntimeFade(clip.track, fade);
        armed = true;
      }
    }
  }
  file.close();

  playback_arrangement_idx_ = mcl_cfg.active_arrangement_idx;
  playback_active_ = true;
  last_tick_q12_ = positionQ12;
  playback_active_mask_ =
      (uint16_t)((playback_active_mask_ & (uint16_t)~clearMask) | loadMask);
  return armed;
}

void MCLArrangement::armRuntimeFade(uint8_t dst,
                                    const TrackLoadFadeData &fade) {
  if (dst >= 16) {
    return;
  }
  uint16_t bit = (uint16_t)(1u << dst);
  if (!fade.enabled()) {
    clip_runtime_fade_mask_ &= (uint16_t)~bit;
    return;
  }
  clip_runtime_fades_[dst] = fade;
  clip_runtime_fade_mask_ |= bit;
}

bool MCLArrangement::applyClipRuntime(uint8_t dst, DeviceTrack *track) {
  if (dst >= 16 || track == nullptr) {
    return false;
  }
  uint16_t bit = (uint16_t)(1u << dst);
  if ((clip_runtime_fade_mask_ & bit) == 0) {
    return false;
  }
  clip_runtime_fade_mask_ &= (uint16_t)~bit;
  TrackLoadFadeData *fade = track->load_fade_data();
  if (fade == nullptr) {
    return false;
  }
  *fade = clip_runtime_fades_[dst];
  return true;
}

bool MCLArrangement::seekLoad(uint32_t positionQ12, bool immediate,
                              bool allowPrestartFade) {
  playback_arrangement_idx_ = mcl_cfg.active_arrangement_idx;
  playback_active_ = true;
  last_tick_q12_ = positionQ12;
  clip_runtime_fade_mask_ = 0;
  uint32_t endQ12 = positionQ12 == 0xFFFFFFFFu ? positionQ12
                                                : positionQ12 + 1u;
  uint8_t queueFlags = immediate ? LOAD_QUEUE_FLAG_IMMEDIATE : 0;
  if (allowPrestartFade) {
    queueFlags |= LOAD_QUEUE_FLAG_PRESTART_FADE;
  }
  return queueClipStarts(positionQ12, endQ12, true, true, queueFlags);
}

bool MCLArrangement::queueClipStarts(uint32_t startQ12, uint32_t endQ12,
                                     bool loadActiveAtPosition,
                                     bool clearInactiveTracks,
                                     uint8_t loadQueueFlags) {
  File file;
  if (!openActive(&file, O_READ)) {
    return false;
  }

  mclarrfile::Header header;
  bool ok = mcl_sd.read_data(&header, sizeof(header), &file) &&
            mclarrfile::validHeader(header);
  if (!ok || header.clipCount == 0 ||
      !file.seekSet(header.headerBytes)) {
    file.close();
    playback_active_mask_ = 0;
    return false;
  }

  GridRow rows[NUM_SLOTS];
  memset(rows, 255, sizeof(rows));
  bool any = false;
  bool hasPlayableClip = false;
  uint16_t currentActiveMask = 0;
  uint16_t startMask = 0;
  uint32_t nowQ12 = endQ12 > startQ12 ? endQ12 - 1u : startQ12;
  for (uint32_t i = 0; i < header.clipCount; ++i) {
    mclarrfile::Clip clip;
    if (!readClipRecord(file, header, clip)) {
      break;
    }
    if ((clip.flags & mclarrfile::CLIP_MUTED) != 0 ||
        clip.durationQ12 == 0 || clip.track >= 16 ||
        clip.row >= GRID_LENGTH) {
      continue;
    }
    hasPlayableClip = true;
    uint64_t clipStart = clip.startQ12;
    uint64_t clipEnd = clipStart + clip.durationQ12;
    if (clipEnd < clipStart) {
      clipEnd = 0xFFFFFFFFull;
    }
    if ((uint64_t)nowQ12 >= clipStart && (uint64_t)nowQ12 < clipEnd) {
      currentActiveMask |= (uint16_t)(1u << clip.track);
      if (loadActiveAtPosition) {
        rows[clip.track] = clip.row;
        startMask |= (uint16_t)(1u << clip.track);
        TrackLoadFadeData fade;
        if (clipFadeAtPosition(clip, nowQ12, fade)) {
          armRuntimeFade(clip.track, fade);
        }
        any = true;
      }
    }
    if (q12InRange(clip.startQ12, startQ12, endQ12)) {
      rows[clip.track] = clip.row;
      startMask |= (uint16_t)(1u << clip.track);
      TrackLoadFadeData fade;
      if (clipFadeAtPosition(clip, clip.startQ12, false, fade)) {
        armRuntimeFade(clip.track, fade);
      }
      any = true;
    }
    if ((clip.flags & mclarrfile::CLIP_FADE_OVERRIDE) != 0) {
      TrackLoadFadeData fade = clipFadeData(clip, true);
      if (fade.enabled() && fade.fade_out()) {
        uint64_t fadeDuration =
            fade.duration_q12 < clip.durationQ12 ? fade.duration_q12
                                                 : clip.durationQ12;
        uint64_t fadeStart =
            clipEnd > fadeDuration ? clipEnd - fadeDuration : clipStart;
        if (fadeDuration > 0 && fadeStart != clipStart &&
            fadeStart <= 0xFFFFFFFFull &&
            q12InRange((uint32_t)fadeStart, startQ12, endQ12) &&
            clipFadeAtPosition(clip, (uint32_t)fadeStart, fade)) {
          rows[clip.track] = clip.row;
          startMask |= (uint16_t)(1u << clip.track);
          armRuntimeFade(clip.track, fade);
          any = true;
        }
      }
    }
  }
  file.close();

  if (!hasPlayableClip) {
    playback_active_mask_ = 0;
    return false;
  }

  uint16_t clearBaseMask =
      clearInactiveTracks ? (uint16_t)0xFFFF : playback_active_mask_;
  uint16_t clearMask = clearBaseMask & (uint16_t)~currentActiveMask;
  clearMask &= (uint16_t)~startMask;
  for (uint8_t track = 0; track < NUM_SLOTS; ++track) {
    if (((clearMask >> track) & 1u) == 0) {
      continue;
    }
    rows[track] = LOAD_QUEUE_CLEAR_ROW;
    any = true;
  }
  playback_active_mask_ = currentActiveMask;

  if (any) {
    grid_task.load_queue.put(LOAD_MANUAL | loadQueueFlags, rows);
  }
  return any;
}

void MCLArrangement::tick() {
  if (!proj.project_loaded || MidiClock.state != MidiClockClass::STARTED) {
    resetPlayback();
    return;
  }

  const uint8_t activeIdx = mcl_cfg.active_arrangement_idx;
  const uint32_t nowQ12 = currentClockQ12();
  uint32_t startQ12 = nowQ12;
  uint32_t endQ12 = nowQ12 + 1u;

  bool sameArrangement =
      playback_active_ && playback_arrangement_idx_ == activeIdx;
  if (sameArrangement) {
    if (nowQ12 == last_tick_q12_) {
      return;
    }
    if (nowQ12 > last_tick_q12_ &&
        nowQ12 - last_tick_q12_ <= kMaxPlaybackCatchupQ12) {
      startQ12 = last_tick_q12_ + 1u;
      endQ12 = nowQ12 + 1u;
    }
  } else {
    playback_active_mask_ = 0;
  }

  playback_active_ = true;
  playback_arrangement_idx_ = activeIdx;
  last_tick_q12_ = nowQ12;
  queueClipStarts(startQ12, endQ12, false, false,
                  LOAD_QUEUE_FLAG_IMMEDIATE);
}

#endif // !defined(__AVR__)
