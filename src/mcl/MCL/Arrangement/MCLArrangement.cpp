#include "MCLPlatformFeatures.h"

#if MCL_FEATURE_HOST_ARRANGER

#include "Arrangement/MCLArrangement.h"
#include "MCLArrangement_Internal.h"

using namespace mcl_arrangement_internal;

MCLArrangement mcl_arrangement;

bool MCLArrangement::openIndex(File *file, uint8_t idx, int mode) {
  if (file == nullptr || !enterProjectDir()) {
    return false;
  }
  char path[16];
  if (!buildRelativePath(idx, path, sizeof(path))) {
    return false;
  }
  return file->open(path, mode);
}

bool MCLArrangement::openActive(File *file, int mode) {
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
    clearLoopRegion();
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
    clearLoopRegion();
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

#endif  // MCL_FEATURE_HOST_ARRANGER
