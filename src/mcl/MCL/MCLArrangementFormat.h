/**
 * MCLArrangementFormat.h - raw MCL arrangement file format.
 *
 * Keep the host and MCL copies byte-identical.
 *
 * Arrangement files are project-local:
 *   /Projects/<project>/Arr/NNN.arr
 *
 * The file stores an explicit per-track timeline. Timing is frozen at import
 * time; clip track/row still references live grid slot content.
 */
#ifndef MCL_ARRANGEMENT_FORMAT_H
#define MCL_ARRANGEMENT_FORMAT_H

#include <stdint.h>
#include <stddef.h>

namespace mclarrfile {

static const uint16_t kVersion = 1;
static const uint8_t kNameBytes = 16;
static const uint8_t kFilenameDigits = 3;
static const uint8_t kMaxArrangementIndex = 255;

static const char kDirName[] = "Arr";
static const char kExtension[] = ".arr";

enum ClipFlags {
    CLIP_LOOP = 1 << 0,
    CLIP_MUTED = 1 << 1,
    CLIP_LOAD_SOUND = 1 << 2
};

struct Header {
    char magic[4];          // "SPAR"
    uint16_t version;
    uint16_t headerBytes;
    uint16_t clipBytes;
    uint16_t flags;
    uint32_t clipCount;
    char name[kNameBytes];  // display name, NUL-padded
};

struct Clip {
    uint32_t startQ12;
    uint32_t durationQ12;
    uint32_t repeatQ12;
    uint8_t track;
    uint8_t row;
    uint8_t flags;
    uint8_t reserved;
};

static_assert(sizeof(Header) == 32, "MCL arrangement header layout changed");
static_assert(sizeof(Clip) == 16, "MCL arrangement clip layout changed");

inline void initHeader(Header& h, const char* name) {
    h.magic[0] = 'S';
    h.magic[1] = 'P';
    h.magic[2] = 'A';
    h.magic[3] = 'R';
    h.version = kVersion;
    h.headerBytes = (uint16_t)sizeof(Header);
    h.clipBytes = (uint16_t)sizeof(Clip);
    h.flags = 0;
    h.clipCount = 0;
    for (uint8_t i = 0; i < kNameBytes; ++i) {
        h.name[i] = '\0';
    }
    if (name == nullptr) {
        return;
    }
    uint8_t i = 0;
    while (i + 1 < kNameBytes && name[i] != '\0') {
        h.name[i] = name[i];
        ++i;
    }
}

inline bool validHeader(const Header& h) {
    return h.magic[0] == 'S' && h.magic[1] == 'P' &&
           h.magic[2] == 'A' && h.magic[3] == 'R' &&
           h.version == kVersion &&
           h.headerBytes >= sizeof(Header) &&
           h.clipBytes == sizeof(Clip);
}

} // namespace mclarrfile

#endif // MCL_ARRANGEMENT_FORMAT_H
