/**
 * MCLArrangementFormat.h - raw MCL arrangement file format.
 *
 * Keep the host and MCL copies byte-identical.
 *
 * Arrangement files are project-local:
 *   /Projects/<project>/Arr/NNN.arr
 *
 * The file stores an explicit per-track timeline. Timing is frozen at import
 * time; clip sources may reference live grid slot content or arranger-private
 * material.
 */
#ifndef MCL_ARRANGEMENT_FORMAT_H
#define MCL_ARRANGEMENT_FORMAT_H

#include <stdint.h>
#include <stddef.h>

namespace mclarrfile {

static const uint16_t kVersion = 7;
static const uint16_t kMinVersion = 1;
static const uint16_t kClipBytesV1 = 16;
static const uint16_t kClipBytesV2 = 24;
static const uint16_t kClipBytesV4 = 32;
static const uint16_t kClipBytesV5 = 40;
static const uint8_t kNameBytes = 16;
static const uint8_t kMarkerLabelBytes = 16;
static const uint8_t kTrackLabelBytes = 16;
static const uint8_t kTrackLabelCount = 16;
static const uint8_t kLoopRegionLabelBytes = 16;
static const uint8_t kFilenameDigits = 3;
static const uint8_t kMaxArrangementIndex = 255;
static const uint16_t kMaxMarkers = 128;
static const uint16_t kMaxLoopRegions = 64;
static const uint16_t kMaxAutomationLanes = 256;
static const uint32_t kMaxAutomationPoints = 8192;
// Arrangement loop repeatCount: 0 means infinite, matching song loop rows.
static const uint16_t kLoopRegionRepeatInfinite = 0;
static const uint8_t kGridSourceSlotCount = 32;

static const char kDirName[] = "Arr";
static const char kExtension[] = ".arr";

enum ClipFlags {
    CLIP_LOOP = 1 << 0,
    CLIP_MUTED = 1 << 1,
    CLIP_LOAD_SOUND = 1 << 2,
    CLIP_FADE_OVERRIDE = 1 << 3
};

enum ClipSourceKind {
    CLIP_SOURCE_GRID = 0,
    CLIP_SOURCE_PRIVATE = 1
};

enum HeaderFlags {
    HEADER_HAS_MARKERS = 1 << 0,
    HEADER_HAS_TRACK_LABELS = 1 << 1,
    HEADER_HAS_LOOP_REGIONS = 1 << 2,
    HEADER_HAS_AUTOMATION = 1 << 3,
    HEADER_HAS_CHUNKS = 1 << 15
};

enum MarkerFlags {
    MARKER_LABEL = 1 << 0
};

enum LoopRegionFlags {
    LOOP_REGION_ENABLED = 1 << 0
};

enum AutomationTargetType {
    AUTOMATION_TARGET_MD_PARAM = 0,
    AUTOMATION_TARGET_MUTE = 1,
    AUTOMATION_TARGET_FILL = 2,
    AUTOMATION_TARGET_PERF = 3,
    AUTOMATION_TARGET_ROUTING = 4,
    AUTOMATION_TARGET_TEMPO = 5
};

enum AutomationValueType {
    AUTOMATION_VALUE_U7 = 0,
    AUTOMATION_VALUE_BOOL = 1,
    AUTOMATION_VALUE_U14 = 2
};

enum AutomationLaneFlags {
    AUTOMATION_LANE_ENABLED = 1 << 0
};

enum AutomationPointInterp {
    AUTOMATION_INTERP_HOLD = 0,
    AUTOMATION_INTERP_CURVE = 1
};

static inline uint32_t chunkId(char a, char b, char c, char d) {
    return ((uint32_t)(uint8_t)a) | ((uint32_t)(uint8_t)b << 8) |
           ((uint32_t)(uint8_t)c << 16) | ((uint32_t)(uint8_t)d << 24);
}

static const uint32_t CHUNK_CLIPS = 0x50494c43u;      // "CLIP"
static const uint32_t CHUNK_MARKERS = 0x4b52414du;    // "MARK"
static const uint32_t CHUNK_TRACK_LABELS = 0x42414c54u; // "TLAB"
static const uint32_t CHUNK_LOOP_REGIONS = 0x504f4f4cu; // "LOOP"
static const uint32_t CHUNK_AUTOMATION_LANES = 0x4e414c41u; // "ALAN"
static const uint32_t CHUNK_AUTOMATION_POINTS = 0x544e5041u; // "APNT"

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

struct HeaderExtraV6 {
    uint16_t markerBytes;
    uint16_t markerCount;
    uint16_t trackLabelBytes;
    uint16_t trackLabelCount;
    uint16_t loopRegionBytes;
    uint16_t loopRegionCount;
};

struct HeaderExtraV7 {
    uint16_t markerBytes;
    uint16_t markerCount;
    uint16_t trackLabelBytes;
    uint16_t trackLabelCount;
    uint16_t loopRegionBytes;
    uint16_t loopRegionCount;
    uint16_t chunkDirBytes;
    uint16_t chunkCount;
};

struct ChunkDirEntry {
    uint32_t id;
    uint32_t offset;
    uint32_t count;
    uint16_t itemBytes;
    uint16_t flags;
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
    uint8_t sourceKind;
    uint8_t sourceTrack;     // Absolute source GridSlot 0..31; kept named for V5 compatibility.
    uint8_t sourceFlags;
    uint8_t sourceReserved;
    uint32_t sourceId;
};

struct Marker {
    uint32_t startQ12;
    uint8_t track;          // 0..15 lane marker, 255 global marker
    uint8_t flags;
    char label[kMarkerLabelBytes];
    uint16_t reserved;
};

struct LoopRegion {
    uint32_t startQ12;
    uint32_t endQ12;
    uint16_t repeatCount;
    uint16_t id;
    uint8_t flags;
    uint8_t reserved[3];
    char label[kLoopRegionLabelBytes];
};

struct AutomationLane {
    uint8_t track;
    uint8_t targetType;
    uint8_t device;
    uint8_t targetIndex;
    uint8_t valueType;
    uint8_t flags;
    uint16_t pointCount;
    uint32_t pointOffset;
    uint32_t reserved;
};

struct AutomationPoint {
    uint32_t q12;
    uint16_t value;
    uint8_t interp;
    int8_t curve;
};

static_assert(sizeof(Header) == 32, "MCL arrangement header layout changed");
static_assert(sizeof(HeaderExtraV2) == 8, "MCL arrangement header extra layout changed");
static_assert(sizeof(HeaderExtraV6) == 12, "MCL arrangement header extra v6 layout changed");
static_assert(sizeof(HeaderExtraV7) == 16, "MCL arrangement header extra v7 layout changed");
static_assert(sizeof(ChunkDirEntry) == 16, "MCL arrangement chunk directory layout changed");
static_assert(sizeof(Clip) == 40, "MCL arrangement clip layout changed");
static_assert(sizeof(Marker) == 24, "MCL arrangement marker layout changed");
static_assert(sizeof(LoopRegion) == 32, "MCL arrangement loop region layout changed");
static_assert(sizeof(AutomationLane) == 16, "MCL arrangement automation lane layout changed");
static_assert(sizeof(AutomationPoint) == 8, "MCL arrangement automation point layout changed");

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

inline void initGridSource(Clip& clip, uint8_t sourceSlot) {
    clip.sourceKind = CLIP_SOURCE_GRID;
    clip.sourceTrack = sourceSlot;
    clip.sourceFlags = 0;
    clip.sourceReserved = 0;
    clip.sourceId = 0;
}

inline uint8_t encodePrivateSourceSlot(uint8_t sourceSlot) {
    return sourceSlot < kGridSourceSlotCount ? (uint8_t)(sourceSlot + 1) : 0;
}

inline bool decodePrivateSourceSlot(uint8_t sourceFlags,
                                    uint8_t& sourceSlot) {
    if (sourceFlags == 0 || sourceFlags > kGridSourceSlotCount) {
        return false;
    }
    sourceSlot = (uint8_t)(sourceFlags - 1);
    return true;
}

inline uint8_t clipPrivateSourceSlot(const Clip& clip) {
    uint8_t sourceSlot = 0;
    if (decodePrivateSourceSlot(clip.sourceFlags, sourceSlot)) {
        return sourceSlot;
    }
    return clip.track;
}

inline uint8_t clipSourceSlot(const Clip& clip) {
    if (clip.sourceKind == CLIP_SOURCE_PRIVATE) {
        return clipPrivateSourceSlot(clip);
    }
    return clip.sourceTrack < kGridSourceSlotCount ? clip.sourceTrack : clip.track;
}

inline uint8_t clipSourceTrack(const Clip& clip) {
    return clipSourceSlot(clip);
}

inline bool validHeader(const Header& h) {
    return h.magic[0] == 'S' && h.magic[1] == 'P' &&
           h.magic[2] == 'A' && h.magic[3] == 'R' &&
           h.version >= kMinVersion &&
           h.version <= kVersion &&
           h.headerBytes >= sizeof(Header) &&
           (h.clipBytes == sizeof(Clip) || h.clipBytes == kClipBytesV4 ||
            h.clipBytes == kClipBytesV2 || h.clipBytes == kClipBytesV1);
}

} // namespace mclarrfile

#endif // MCL_ARRANGEMENT_FORMAT_H
