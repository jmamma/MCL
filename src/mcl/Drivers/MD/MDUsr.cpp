#include "MDUsr.h"
#include "MD.h"
#include "MCLMemory.h"

MDUsrCache md_usr;

#if defined(__AVR__)
// The bank-zero globals end almost at 0xffff. Use the unallocated bank-one
// tail, after the existing track caches; never bank-switch over a global copy.
static constexpr uint32_t kUsrCache = BANK1_USR_CACHE_START;
static_assert(BANK1_USR_CACHE_LEN == MD_USR_SLOTS * MD_USR_RECORD_BYTES,
              "USR metadata layout differs from the reserved SRAM region");
static_assert(kUsrCache + MD_USR_SLOTS * MD_USR_RECORD_BYTES <= 0x10000UL,
              "USR cache overlaps the end of AVR bank one");
#endif

uint8_t MDUsrCache::readByte(uint16_t offset) const {
#if defined(__AVR__)
  return get_byte_bank1(reinterpret_cast<volatile uint8_t *>(uint16_t(kUsrCache + offset)));
#else
  return records_[offset];
#endif
}

void MDUsrCache::writeByte(uint16_t offset, uint8_t value) {
#if defined(__AVR__)
  put_byte_bank1(reinterpret_cast<volatile uint8_t *>(uint16_t(kUsrCache + offset)), value);
#else
  records_[offset] = value;
#endif
}

void MDUsrCache::reset() {
  valid_pages_ = 0x80;
  slot_count_ = 0;
  pending_ = 255;
  kit_received_ = kit_dirty_ = false;
  retries_ = 0;
}

bool MDUsrCache::request(uint8_t page, uint8_t kit) {
  if (!(MD.fw_caps & FW_CAP_USR_QUERY)) return false;
  if (++tag_ == 127) tag_ = 0; // $7f is reserved for unsolicited notices.
  uint8_t message[] = {0x70, MD_USR_QUERY, MD_USR_VERSION, tag_, page, kit};
  pending_ = page;
  requested_at_ = read_clock_ms();
  if (MD.sendRequest(message, sizeof(message)) != 0) return true;
  pending_ = 255;
  return false;
}

void MDUsrCache::kitChanged() {
  // Called by the MIDI listener. All requests stay in the foreground loop.
  kit_dirty_ = true;
  kit_received_ = false;
  pending_ = 255;
  retries_ = 0;
}

void MDUsrCache::retry() {
  requested_at_ = read_clock_ms();
  if (++retries_ < 3) {
    pending_ = 254; // One-second delay between bounded delivery retries.
  } else {
    pending_ = 255;
    kit_dirty_ = false;
    // Wait for the next notification, reconnect or explicit kit read.
    slot_count_ = 0;
    valid_pages_ = 0x87;
  }
}

void MDUsrCache::service() {
  if (!MD.connected || !(MD.fw_caps & FW_CAP_USR_QUERY)) return;
  if (pending_ != 255) {
    if (uint16_t(read_clock_ms() - requested_at_) < 1000) return;
    if (pending_ != 254) {
      retry();
      return;
    }
    pending_ = 255;
  }
  if (kit_dirty_) {
    kit_dirty_ = false;
    if (!MD.getBlockingKit(127, 1000)) {
      // A newer event during the read already scheduled a fresh attempt.
      if (!kit_dirty_) { kit_dirty_ = true; retry(); }
      return;
    }
    retries_ = 0;
    if (kit_dirty_) return;
  }
  for (uint8_t page = 0; page < 3; ++page) {
    if (!(valid_pages_ & (1u << page))) {
      if (!request(page)) retry();
      return;
    }
  }
  // Cache is complete. Further reads are driven by MD notifications.
}

bool MDUsrCache::readKitTuning(uint8_t kit, uint16_t timeout) {
  if (!(MD.fw_caps & FW_CAP_USR_QUERY)) return true;
  kit_received_ = false;
  if (!request(MD_USR_KIT_PAGE, kit)) return false;
  // Called only as the second part of an already blocking kit read. The
  // ordinary foreground loop owns all background metadata requests.
  while (pending_ != 255) {
    const uint16_t elapsed = uint16_t(read_clock_ms() - requested_at_);
    if (elapsed >= timeout) break;
    MD.waitBlocking(timeout - elapsed);
  }
  pending_ = 255;
  return kit_received_;
}

void MDUsrCache::receive(const SysexView &view) {
  // Excludes F0/F7: header[5], 72, command, version, tag, page, count,
  // five little-endian 7-bit generation bytes, then fixed-size records.
  if (view.get_recordLen() < 16 || view.getByte(7) != MD_USR_VERSION ||
      view.getByte(15) > 15) return;
  const uint8_t count = view.getByte(10), page = view.getByte(9);
  if (count != 0 && count != 16 && count != 48) return;
  uint32_t generation = 0;
  for (uint8_t n = 0; n < 5; ++n)
    generation |= uint32_t(view.getByte(11 + n)) << (n * 7);
  if (view.getByte(8) == 127) {
    if (page != MD_USR_STATUS_PAGE || view.get_recordLen() != 16) return;
    // Also catches same-slot replacements and changes to unassigned machines.
    invalidate();
    generation_ = generation;
    slot_count_ = count;
    pending_ = 255;
    retries_ = 0;
    if (count == 0) valid_pages_ |= 7;
    return;
  }
  if (pending_ >= 5 || view.getByte(8) != tag_ || page != pending_) return;
  if (count == 0 && view.get_recordLen() == 16) {
    // Try a busy peer briefly, then wait for its completion notice.
    slot_count_ = 0;
    invalidate();
    retry();
    return;
  }
  const uint8_t slots = page < 3 && page * 16 < count ? 16 : 0;
  const uint8_t bytes = page == MD_USR_KIT_PAGE ? 32 : slots * MD_USR_RECORD_BYTES;
  if (view.get_recordLen() != uint16_t(16 + bytes)) return;
  if (page < 3) {
    for (uint8_t record = 16; record < 16 + bytes; record += MD_USR_RECORD_BYTES) {
      const uint8_t flags = view.getByte(record);
      if ((flags & 0x40) || ((flags & MD_USR_TONAL) && !(flags & MD_USR_PITCH)) ||
          (!(flags & MD_USR_PRESENT) && flags != 0)) return;
      for (uint8_t n = 1; n < MD_USR_RECORD_BYTES; ++n) {
        const uint8_t value = view.getByte(record + n);
        if (value != 0 && (value < 32 || value > 126)) return;
      }
    }
  } else if (page == MD_USR_KIT_PAGE) {
    for (uint8_t track = 0; track < 16; ++track) {
      const uint32_t model = mdModelFromMachineFlags(view.getByte(16 + track * 2),
                                                    view.getByte(17 + track * 2));
      if (uint8_t(model) != MD.kit.get_model(track)) return;
    }
  }
  if (generation != generation_ || count != slot_count_) invalidate();
  generation_ = generation;
  slot_count_ = count;
  if (page < 3) {
    const uint16_t offset = uint16_t(page) * 16 * MD_USR_RECORD_BYTES;
    for (uint8_t n = 0; n < bytes; ++n) writeByte(offset + n, view.getByte(16 + n));
    valid_pages_ |= 1u << page;
    if (count == 16) valid_pages_ |= 6;
    if (MD.currentTrack < 16 && uint8_t(MD.kit.get_model(MD.currentTrack) - MD_USR_FIRST) / 16 == page)
      valid_pages_ |= 0x80;
  } else if (page == MD_USR_KIT_PAGE) {
    for (uint8_t track = 0; track < 16; ++track)
      MD.kit.models[track] = mdModelFromMachineFlags(view.getByte(16 + track * 2),
                                                    view.getByte(17 + track * 2));
    kit_received_ = true;
  }
  pending_ = 255;
  retries_ = 0;
}

uint8_t MDUsrCache::descriptor(uint8_t model) const {
  const uint8_t slot = model - MD_USR_FIRST;
  return slot < MD_USR_SLOTS && slot < slot_count_ && (valid_pages_ & (1u << (slot / 16)))
      ? readByte(uint16_t(slot) * MD_USR_RECORD_BYTES) : 0;
}

const char *MDUsrCache::name(uint8_t model) {
  if (!(descriptor(model) & MD_USR_PRESENT)) return nullptr;
  const uint16_t offset = uint16_t(model - MD_USR_FIRST) * MD_USR_RECORD_BYTES + 1;
  for (uint8_t n = 0; n < 5; ++n) name_[n < 3 ? n : n + 1] = readByte(offset + n);
  name_[3] = '-';
  name_[6] = 0;
  return name_;
}
