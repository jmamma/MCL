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

static const uint16_t kVersion = 4;
static const uint16_t kMinVersion = 1;
static const uint16_t kClipBytesV1 = 16;
static const uint16_t kClipBytesV2 = 24;
static const uint8_t kNameBytes = 16;
static const uint8_t kMarkerLabelBytes = 16;
static const uint8_t kTrackLabelBytes = 16;
static const uint8_t kTrackLabelCount = 16;
static const uint8_t kFilenameDigits = 3;
static const uint8_t kMaxArrangementIndex = 255;
static const uint16_t kMaxMarkers = 128;

static const char kDirName[] = "Arr";
static const char kExtension[] = ".arr";

enum ClipFlags {
    CLIP_LOOP = 1 << 0,
    CLIP_MUTED = 1 << 1,
    CLIP_LOAD_SOUND = 1 << 2,
    CLIP_FADE_OVERRIDE = 1 << 3
};

enum HeaderFlags {
    HEADER_HAS_MARKERS = 1 << 0,
    HEADER_HAS_TRACK_LABELS = 1 << 1
};

enum MarkerFlags {
    MARKER_LABEL = 1 << 0
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

struct HeaderExtraV2 {
    uint16_t markerBytes;
    uint16_t markerCount;
    uint16_t trackLabelBytes;
    uint16_t trackLabelCount;
};

struct Clip {
    uint32_t startQ12;
    uint32_t durationQ12;
    uint32_t repeatQ12;
    uint8_t track;
    uint8_t row;
    uint8_t flags;
    uint8_t reserved;
    uint8_t fadeFlags;
    uint8_t fadeTarget;
    uint16_t fadeDurationQ12;
    uint8_t fadeAmount;
    int8_t fadeCurve;
    uint16_t fadeReserved;
    uint8_t endFadeFlags;
    uint8_t endFadeTarget;
    uint16_t endFadeDurationQ12;
    uint8_t endFadeAmount;
    int8_t endFadeCurve;
    uint16_t endFadeReserved;
};

struct Marker {
    uint32_t startQ12;
    uint8_t track;          // 0..15 lane marker, 255 global marker
    uint8_t flags;
    char label[kMarkerLabelBytes];
    uint16_t reserved;
};

static_assert(sizeof(Header) == 32, "MCL arrangement header layout changed");
static_assert(sizeof(HeaderExtraV2) == 8, "MCL arrangement header extra layout changed");
static_assert(sizeof(Clip) == 32, "MCL arrangement clip layout changed");
static_assert(sizeof(Marker) == 24, "MCL arrangement marker layout changed");

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
           h.version >= kMinVersion &&
           h.version <= kVersion &&
           h.headerBytes >= sizeof(Header) &&
           (h.clipBytes == sizeof(Clip) || h.clipBytes == kClipBytesV2 ||
            h.clipBytes == kClipBytesV1);
}

} // namespace mclarrfile

#endif // MCL_ARRANGEMENT_FORMAT_H
