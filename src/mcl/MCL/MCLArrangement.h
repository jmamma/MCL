/**
 * MCLArrangement - project-local arrangement storage.
 */
#pragma once

#if !defined(__AVR__)

#include "MCLArrangementFormat.h"
#include <stdint.h>

class MCLArrangement {
public:
  bool ensureActive();
  bool readMeta(mclarrfile::Header *header);
  bool clearActive();
  bool saveActive();
  bool select(uint8_t idx);
  bool create(uint8_t idx, const char *name);
  bool createFirst(uint8_t *idxOut);
  bool importGrid(uint16_t trackMask, uint8_t startRow);
  void tick();
  void resetPlayback();

  uint16_t readClips(uint32_t startQ12, uint32_t endQ12, uint16_t skip,
                     uint8_t maxClips, mclarrfile::Clip *out,
                     uint32_t *totalMatches = nullptr,
                     bool *more = nullptr);

private:
  bool openActive(class FsFile *file, uint8_t mode);
  bool openIndex(class FsFile *file, uint8_t idx, uint8_t mode);
  bool queueClipStarts(uint32_t startQ12, uint32_t endQ12);
  bool rewriteActive(const mclarrfile::Header &header,
                     const mclarrfile::Clip *clips, uint32_t clipCount);

  uint8_t playback_arrangement_idx_ = 0xFF;
  uint32_t last_tick_q12_ = 0;
  bool playback_active_ = false;
};

extern MCLArrangement mcl_arrangement;

#endif // !defined(__AVR__)
