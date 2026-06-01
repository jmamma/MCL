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
  return cell;
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
  if (!ensureArrangementDir()) {
    return false;
  }
  File file;
  if (!openActive(&file, O_RDWR | O_CREAT)) {
    return false;
  }
  mclarrfile::Header outHeader = header;
  outHeader.version = mclarrfile::kVersion;
  outHeader.headerBytes = sizeof(mclarrfile::Header);
  outHeader.clipBytes = sizeof(mclarrfile::Clip);
  outHeader.clipCount = clipCount;
  bool ok = file.seekSet(0) &&
            mcl_sd.write_data(&outHeader, sizeof(outHeader), &file);
  for (uint32_t i = 0; ok && i < clipCount; ++i) {
    ok = mcl_sd.write_data((void *)&clips[i], sizeof(clips[i]), &file);
  }
  uint32_t fileSize = sizeof(mclarrfile::Header) +
                      clipCount * sizeof(mclarrfile::Clip);
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
  bool ok = rewriteActive(header, nullptr, 0);
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
    if (!mcl_sd.read_data(&clip, sizeof(clip), &file)) {
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
  playback_active_ = false;
}

bool MCLArrangement::queueClipStarts(uint32_t startQ12, uint32_t endQ12) {
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
    return false;
  }

  GridRow rows[NUM_SLOTS];
  memset(rows, 255, sizeof(rows));
  bool any = false;
  for (uint32_t i = 0; i < header.clipCount; ++i) {
    mclarrfile::Clip clip;
    if (!mcl_sd.read_data(&clip, sizeof(clip), &file)) {
      break;
    }
    if ((clip.flags & mclarrfile::CLIP_MUTED) != 0 ||
        clip.durationQ12 == 0 || clip.track >= NUM_SLOTS ||
        clip.row >= GRID_LENGTH ||
        !q12InRange(clip.startQ12, startQ12, endQ12)) {
      continue;
    }
    rows[clip.track] = clip.row;
    any = true;
  }
  file.close();

  if (any) {
    grid_task.load_queue.put(LOAD_MANUAL, rows);
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

  if (playback_active_ && playback_arrangement_idx_ == activeIdx) {
    if (nowQ12 == last_tick_q12_) {
      return;
    }
    if (nowQ12 > last_tick_q12_ &&
        nowQ12 - last_tick_q12_ <= kMaxPlaybackCatchupQ12) {
      startQ12 = last_tick_q12_ + 1u;
      endQ12 = nowQ12 + 1u;
    }
  }

  playback_active_ = true;
  playback_arrangement_idx_ = activeIdx;
  last_tick_q12_ = nowQ12;
  queueClipStarts(startQ12, endQ12);
}

#endif // !defined(__AVR__)
