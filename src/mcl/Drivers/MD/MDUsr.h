#pragma once

#include <stdint.h>

class SysexView;

// 0x70/0x44, version 1. Firmware peer: MegaCommandGateway.cpp CMD_QUERY_USR.
enum : uint8_t {
  MD_USR_QUERY = 0x44, MD_USR_VERSION = 1,
  MD_USR_FIRST = 194, MD_USR_SLOTS = 48, MD_USR_RECORD_BYTES = 6,
  MD_USR_PAGE_SLOTS = 16, MD_USR_KIT_PAGE = 3, MD_USR_STATUS_PAGE = 4,
  MD_USR_PITCH = 8, MD_USR_TONAL = 16, MD_USR_PRESENT = 32
};

class MDUsrCache {
public:
  void reset();
  void service();
  void kitChanged();
  void receive(const SysexView &view);
  bool readKitTuning(uint8_t kit, uint16_t timeout);
  uint8_t descriptor(uint8_t model) const;
  const char *name(uint8_t model);
  void invalidate() { valid_pages_ = 0x80; }
  bool takeChanged() {
    const bool changed = (valid_pages_ & 0x80) != 0;
    valid_pages_ &= 0x7f;
    return changed;
  }

private:
  bool request(uint8_t page, uint8_t kit = 127);
  void retry();
  uint8_t readByte(uint16_t offset) const;
  void writeByte(uint16_t offset, uint8_t value);
  uint32_t generation_ = 0;
  uint16_t requested_at_ = 0;
  uint8_t valid_pages_ = 0, slot_count_ = 0, pending_ = 255, tag_ = 0;
  uint8_t retries_ = 0;
  bool kit_received_ = false, kit_dirty_ = false;
  char name_[7]{};
#if !defined(__AVR__)
  uint8_t records_[MD_USR_SLOTS * MD_USR_RECORD_BYTES]{};
#endif
};

extern MDUsrCache md_usr;
